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

#include <algorithm>
#include <memory>
#include <stdexcept>

#include "graph/custom_op.h"
#include "graph/custom_op_factory.h"
#include "graph/custom_op_registry.h"

namespace ge {
namespace {
class TestPriorityOp final : public BaseCustomOp {};

class TestPriorityOtherOp final : public BaseCustomOp {};

class TestPriorityRegisteredOp final : public BaseCustomOp {};
REG_OP_WITH_PRIORITY(TestPriorityRegisteredOp, "CustomOpRegistryPriorityMacroOp", OpBackend::kDevice,
                     OpRegistrationPriority::kBottom, OpEngine::kAiCore);

class TestPriorityShapeInferOp final : public ShapeInferOp {
 public:
  graphStatus InferShape(gert::InferShapeContext *) override {
    return GRAPH_SUCCESS;
  }

  graphStatus InferDataType(gert::InferDataTypeContext *) override {
    return GRAPH_SUCCESS;
  }
};
REG_OP_WITH_PRIORITY(TestPriorityShapeInferOp, "CustomOpRegistryCapabilityMacroOp", OpBackend::kDevice,
                     OpRegistrationPriority::kBottom, OpEngine::kAiCore);

class TestPriorityOtherShapeInferOp final : public ShapeInferOp {
 public:
  graphStatus InferShape(gert::InferShapeContext *) override {
    return GRAPH_SUCCESS;
  }

  graphStatus InferDataType(gert::InferDataTypeContext *) override {
    return GRAPH_SUCCESS;
  }
};

class TestDefaultRegisteredOp final : public BaseCustomOp {};
REG_OP_BACKEND(TestDefaultRegisteredOp, "CustomOpRegistryDefaultMacroOp", OpBackend::kHostCPU);

// kHccl引擎注册 + ArgsUpdater能力（对齐HcclOp的注册形态）
class TestHcclArgsUpdaterOp final : public ArgsUpdater {
 public:
  graphStatus UpdateHostArgs(gert::UpdateArgsContext *ctx) override {
    (void)ctx;
    return GRAPH_SUCCESS;
  }
};

std::unique_ptr<BaseCustomOp> MakePriorityOp() {
  return std::make_unique<TestPriorityOp>();
}

std::unique_ptr<BaseCustomOp> MakeOtherOp() {
  return std::make_unique<TestPriorityOtherOp>();
}

std::unique_ptr<BaseCustomOp> MakeShapeInferOp() {
  return std::make_unique<TestPriorityShapeInferOp>();
}

std::unique_ptr<BaseCustomOp> MakeOtherShapeInferOp() {
  return std::make_unique<TestPriorityOtherShapeInferOp>();
}

std::unique_ptr<BaseCustomOp> MakeHcclArgsUpdaterOp() {
  return std::make_unique<TestHcclArgsUpdaterOp>();
}

void RegisterOp(CustomOpRegistry &registry, const AscendString &op_type, OpBackend backend,
                OpRegistrationPriority priority, OpEngine engine, const BaseOpCreator &creator) {
  ASSERT_EQ(registry.RegisterCreator(op_type, backend, priority, engine, creator), GRAPH_SUCCESS);
}
}  // namespace

TEST(CustomOpRegistryPriorityTest, RegisterAndQueryByEngineBackendAndPriority) {
  CustomOpRegistry registry;
  const AscendString op_type("CustomOpRegistryPriorityRegisterOp");
  const OpEngine engine = OpEngine::kAiCore;
  const auto ret =
      registry.RegisterCreator(op_type, OpBackend::kHostCPU, OpRegistrationPriority::kBottom, engine,
                               []() -> std::unique_ptr<BaseCustomOp> { return std::make_unique<TestPriorityOp>(); });
  ASSERT_EQ(ret, GRAPH_SUCCESS);

  EXPECT_TRUE(registry.HasCreator(op_type, OpBackend::kHostCPU, OpRegistrationPriority::kBottom, engine));
  EXPECT_FALSE(registry.HasCreator(op_type, OpBackend::kHostCPU, OpRegistrationPriority::kTop, engine));
  EXPECT_FALSE(registry.HasCreator(op_type, OpBackend::kDevice, OpRegistrationPriority::kBottom, engine));
  EXPECT_FALSE(
      registry.HasCreator(op_type, OpBackend::kHostCPU, OpRegistrationPriority::kBottom, OpEngine::kVectorCore));
  EXPECT_FALSE(registry.HasCreator(op_type, OpBackend::kHostCPU, OpRegistrationPriority::kTop));
}

TEST(CustomOpRegistryPriorityTest, TopAndBottomCanBeRegisteredIndependently) {
  CustomOpRegistry registry;
  const AscendString op_type("CustomOpRegistryPrioritySelectOp");
  const OpEngine engine = OpEngine::kAiCore;
  ASSERT_EQ(
      registry.RegisterCreator(op_type, OpBackend::kHostCPU, OpRegistrationPriority::kBottom, engine,
                               []() -> std::unique_ptr<BaseCustomOp> { return std::make_unique<TestPriorityOp>(); }),
      GRAPH_SUCCESS);
  ASSERT_EQ(registry.RegisterCreator(
                op_type, OpBackend::kHostCPU, OpRegistrationPriority::kTop, engine,
                []() -> std::unique_ptr<BaseCustomOp> { return std::make_unique<TestPriorityOtherOp>(); }),
            GRAPH_SUCCESS);

  EXPECT_TRUE(registry.HasCreator(op_type, OpBackend::kHostCPU, OpRegistrationPriority::kTop, engine));
  EXPECT_TRUE(registry.HasCreator(op_type, OpBackend::kHostCPU, OpRegistrationPriority::kBottom, engine));
}

TEST(CustomOpRegistryPriorityTest, HandlesAllPriorityEngineBackendCombinations) {
  CustomOpRegistry registry;
  const AscendString op_type("CustomOpRegistryPriorityAllDimensionsOp");
  const auto creator = []() -> std::unique_ptr<BaseCustomOp> { return std::make_unique<TestPriorityOp>(); };

  ASSERT_EQ(
      registry.RegisterCreator(op_type, OpBackend::kDevice, OpRegistrationPriority::kTop, OpEngine::kCustom, creator),
      GRAPH_SUCCESS);
  ASSERT_EQ(
      registry.RegisterCreator(op_type, OpBackend::kHostCPU, OpRegistrationPriority::kTop, OpEngine::kAiCore, creator),
      GRAPH_SUCCESS);
  ASSERT_EQ(registry.RegisterCreator(op_type, OpBackend::kDevice, OpRegistrationPriority::kBottom, OpEngine::kCustom,
                                     creator),
            GRAPH_SUCCESS);
  ASSERT_EQ(registry.RegisterCreator(op_type, OpBackend::kHostCPU, OpRegistrationPriority::kBottom, OpEngine::kAiCore,
                                     creator),
            GRAPH_SUCCESS);

  EXPECT_NE(registry.CreateOrGetCustomOp(op_type, OpBackend::kDevice, OpRegistrationPriority::kTop, OpEngine::kCustom),
            nullptr);
  EXPECT_NE(registry.CreateOrGetCustomOp(op_type, OpBackend::kHostCPU, OpRegistrationPriority::kTop, OpEngine::kAiCore),
            nullptr);
  EXPECT_NE(
      registry.CreateOrGetCustomOp(op_type, OpBackend::kDevice, OpRegistrationPriority::kBottom, OpEngine::kCustom),
      nullptr);
  EXPECT_NE(
      registry.CreateOrGetCustomOp(op_type, OpBackend::kHostCPU, OpRegistrationPriority::kBottom, OpEngine::kAiCore),
      nullptr);

  registry.RemoveCustomOps({op_type});
  EXPECT_FALSE(registry.HasCreator(op_type, OpBackend::kDevice, OpRegistrationPriority::kTop, OpEngine::kCustom));
  EXPECT_FALSE(registry.HasCreator(op_type, OpBackend::kHostCPU, OpRegistrationPriority::kTop, OpEngine::kAiCore));
  EXPECT_FALSE(registry.HasCreator(op_type, OpBackend::kDevice, OpRegistrationPriority::kBottom, OpEngine::kCustom));
  EXPECT_FALSE(registry.HasCreator(op_type, OpBackend::kHostCPU, OpRegistrationPriority::kBottom, OpEngine::kAiCore));
}

TEST(CustomOpRegistryPriorityTest, CreateOrGetKeepsInstancesIndependentByPriority) {
  CustomOpRegistry registry;
  const AscendString op_type("CustomOpRegistryPriorityInstanceOp");
  const OpEngine engine = OpEngine::kAiCore;
  ASSERT_EQ(
      registry.RegisterCreator(op_type, OpBackend::kHostCPU, OpRegistrationPriority::kTop, engine,
                               []() -> std::unique_ptr<BaseCustomOp> { return std::make_unique<TestPriorityOp>(); }),
      GRAPH_SUCCESS);
  ASSERT_EQ(
      registry.RegisterCreator(op_type, OpBackend::kHostCPU, OpRegistrationPriority::kBottom, engine,
                               []() -> std::unique_ptr<BaseCustomOp> { return std::make_unique<TestPriorityOp>(); }),
      GRAPH_SUCCESS);

  auto *top_first = registry.CreateOrGetCustomOp(op_type, OpBackend::kHostCPU, OpRegistrationPriority::kTop, engine);
  auto *top_second = registry.CreateOrGetCustomOp(op_type, OpBackend::kHostCPU, OpRegistrationPriority::kTop, engine);
  auto *bottom = registry.CreateOrGetCustomOp(op_type, OpBackend::kHostCPU, OpRegistrationPriority::kBottom, engine);
  ASSERT_NE(top_first, nullptr);
  EXPECT_EQ(top_first, top_second);
  ASSERT_NE(bottom, nullptr);
  EXPECT_NE(top_first, bottom);
}

TEST(CustomOpRegistryPriorityTest, RejectsInvalidAndDuplicateRegistration) {
  CustomOpRegistry registry;
  const AscendString op_type("CustomOpRegistryPriorityInvalidOp");
  const OpEngine engine = OpEngine::kAiCore;
  EXPECT_EQ(
      registry.RegisterCreator(op_type, OpBackend::kHostCPU, OpRegistrationPriority::kTop, engine, BaseOpCreator()),
      GRAPH_PARAM_INVALID);
  EXPECT_EQ(
      registry.RegisterCreator(op_type, OpBackend::kHostCPU, OpRegistrationPriority::kTop, static_cast<OpEngine>(100U),
                               []() -> std::unique_ptr<BaseCustomOp> { return std::make_unique<TestPriorityOp>(); }),
      GRAPH_PARAM_INVALID);
  ASSERT_EQ(
      registry.RegisterCreator(op_type, OpBackend::kHostCPU, OpRegistrationPriority::kTop, engine,
                               []() -> std::unique_ptr<BaseCustomOp> { return std::make_unique<TestPriorityOp>(); }),
      GRAPH_SUCCESS);
  EXPECT_EQ(
      registry.RegisterCreator(op_type, OpBackend::kHostCPU, OpRegistrationPriority::kTop, engine,
                               []() -> std::unique_ptr<BaseCustomOp> { return std::make_unique<TestPriorityOp>(); }),
      GRAPH_FAILED);
}

TEST(CustomOpRegistryPriorityTest, CreatorExceptionPropagates) {
  CustomOpRegistry registry;
  const AscendString op_type("CustomOpRegistryPriorityExceptionOp");
  const OpEngine engine = OpEngine::kAiCore;
  ASSERT_EQ(
      registry.RegisterCreator(op_type, OpBackend::kHostCPU, OpRegistrationPriority::kBottom, engine,
                               []() -> std::unique_ptr<BaseCustomOp> { throw std::runtime_error("creator failed"); }),
      GRAPH_SUCCESS);

  EXPECT_THROW(
      (void)registry.CreateOrGetCustomOp(op_type, OpBackend::kHostCPU, OpRegistrationPriority::kBottom, engine),
      std::runtime_error);
}

TEST(CustomOpRegistryPriorityTest, MacroRegistersExplicitPriorityCreator) {
  const AscendString op_type("CustomOpRegistryPriorityMacroOp");
  const OpEngine engine = OpEngine::kAiCore;
  EXPECT_TRUE(CustomOpFactory::IsExistOp(op_type, OpBackend::kDevice, OpRegistrationPriority::kBottom, engine));
  EXPECT_NE(CustomOpFactory::CreateOrGetCustomOp(op_type, OpBackend::kDevice, OpRegistrationPriority::kBottom, engine),
            nullptr);
}

TEST(CustomOpRegistryPriorityTest, CommonCapabilityCanQueryExplicitPriority) {
  const AscendString op_type("CustomOpRegistryCapabilityMacroOp");
  const OpEngine engine = OpEngine::kAiCore;
  (void)engine;
  auto *custom_op =
      CustomOpFactory::GetCustomOpCommonCapability<ShapeInferOp>(op_type, OpRegistrationPriority::kBottom);
  EXPECT_NE(custom_op, nullptr);
  EXPECT_EQ(CustomOpFactory::GetCustomOpCommonCapability<ShapeInferOp>(op_type, OpRegistrationPriority::kTop), nullptr);
}

TEST(CustomOpRegistryPriorityTest, SameClassSharesInstanceAcrossEnginesWithinPriority) {
  CustomOpRegistry registry;
  const AscendString op_type("CustomOpRegistryShareInstanceAcrossEngineOp");
  const auto creator = []() -> std::unique_ptr<BaseCustomOp> { return std::make_unique<TestPriorityOp>(); };
  ASSERT_EQ(
      registry.RegisterCreator(op_type, OpBackend::kDevice, OpRegistrationPriority::kTop, OpEngine::kCustom, creator),
      GRAPH_SUCCESS);
  ASSERT_EQ(
      registry.RegisterCreator(op_type, OpBackend::kDevice, OpRegistrationPriority::kTop, OpEngine::kAiCore, creator),
      GRAPH_SUCCESS);

  auto *custom_op =
      registry.CreateOrGetCustomOp(op_type, OpBackend::kDevice, OpRegistrationPriority::kTop, OpEngine::kCustom);
  auto *other_engine_op =
      registry.CreateOrGetCustomOp(op_type, OpBackend::kDevice, OpRegistrationPriority::kTop, OpEngine::kAiCore);
  ASSERT_NE(custom_op, nullptr);
  EXPECT_EQ(custom_op, other_engine_op);
}

TEST(CustomOpRegistryPriorityTest, CommonCapabilityRequiresUniqueProviderAcrossEngines) {
  CustomOpRegistry registry;
  const AscendString op_type("CustomOpRegistryMultiEngineCapabilityOp");
  ASSERT_EQ(registry.RegisterCreator(
                op_type, OpBackend::kDevice, OpRegistrationPriority::kTop, OpEngine::kAiCore,
                []() -> std::unique_ptr<BaseCustomOp> { return std::make_unique<TestPriorityShapeInferOp>(); }),
            GRAPH_SUCCESS);
  ASSERT_EQ(registry.RegisterCreator(
                op_type, OpBackend::kDevice, OpRegistrationPriority::kTop, OpEngine::kVectorCore,
                []() -> std::unique_ptr<BaseCustomOp> { return std::make_unique<TestPriorityOtherShapeInferOp>(); }),
            GRAPH_SUCCESS);

  EXPECT_EQ(CustomOpFactory::GetCustomOpCommonCapability<ShapeInferOp>(op_type), nullptr);
}

TEST(CustomOpRegistryPriorityTest, CommonCapabilityFindsProviderFromAnyEngineWithinPriority) {
  CustomOpRegistry registry;
  const AscendString op_type("CustomOpRegistryCrossEngineCapabilityOp");
  ASSERT_EQ(registry.RegisterCreator(
                op_type, OpBackend::kDevice, OpRegistrationPriority::kTop, OpEngine::kAiCore,
                []() -> std::unique_ptr<BaseCustomOp> { return std::make_unique<TestPriorityShapeInferOp>(); }),
            GRAPH_SUCCESS);

  auto *custom_op = registry.GetCustomOpCommonCapability(op_type, CustomOpCapability::kShapeInfer);
  EXPECT_NE(custom_op, nullptr);
}

TEST(CustomOpRegistryPriorityTest, RegOpBackendUsesCustomEngineTopPriority) {
  const AscendString op_type("CustomOpRegistryDefaultMacroOp");
  EXPECT_TRUE(CustomOpFactory::IsExistOp(op_type));
  EXPECT_TRUE(CustomOpFactory::IsExistOp(op_type, OpBackend::kHostCPU, OpRegistrationPriority::kTop));
  EXPECT_TRUE(
      CustomOpFactory::IsExistOp(op_type, OpBackend::kHostCPU, OpRegistrationPriority::kTop, OpEngine::kCustom));
  EXPECT_FALSE(
      CustomOpFactory::IsExistOp(op_type, OpBackend::kHostCPU, OpRegistrationPriority::kBottom, OpEngine::kCustom));
  EXPECT_FALSE(
      CustomOpFactory::IsExistOp(op_type, OpBackend::kHostCPU, OpRegistrationPriority::kTop, OpEngine::kAiCore));
}
// kHccl引擎 + kBottom兜底注册（对齐HcclOp的注册形态）
REG_OP_WITH_PRIORITY(TestHcclArgsUpdaterOp, "CustomOpRegistryHcclArgsUpdaterOp", OpBackend::kDevice,
                     OpRegistrationPriority::kBottom, OpEngine::kHccl);

TEST(CustomOpRegistryPriorityTest, HcclEngineBottomRegistrationResolvedByClaimingLookups) {
  const AscendString op_type("CustomOpRegistryHcclArgsUpdaterOp");
  // 守卫口径（显式kTop）不可见；认领口径（不带优先级，覆盖全部优先级）可见
  EXPECT_FALSE(CustomOpFactory::IsExistOp(op_type, OpRegistrationPriority::kTop));
  EXPECT_FALSE(CustomOpFactory::IsExistOp(op_type, OpBackend::kDevice, OpRegistrationPriority::kTop));
  EXPECT_TRUE(CustomOpFactory::IsExistOp(op_type));
  EXPECT_FALSE(CustomOpFactory::IsExistOp(AscendString("CustomOpRegistryNoSuchOp_Unregistered")));
  // 认领语义：引擎解析 + 优先级回退可命中kBottom注册
  OpEngine engine = OpEngine::kCustom;
  ASSERT_TRUE(CustomOpFactory::GetOpEngine(op_type, OpBackend::kDevice, engine));
  EXPECT_EQ(engine, OpEngine::kHccl);
  EXPECT_TRUE(
      CustomOpFactory::IsExistOp(op_type, OpBackend::kDevice, OpRegistrationPriority::kBottom, OpEngine::kHccl));
  EXPECT_NE(CustomOpFactory::CreateOrGetCustomOp(op_type, OpBackend::kDevice, engine), nullptr);
  EXPECT_FALSE(
      CustomOpFactory::GetOpEngine(AscendString("CustomOpRegistryNoSuchOp_Engine"), OpBackend::kDevice, engine));
}

namespace {
class TestTopEngineRegisteredOp final : public BaseCustomOp {};
}  // namespace
REG_OP_WITH_PRIORITY(TestTopEngineRegisteredOp, "CustomOpRegistryTopEngineVisibleOp", OpBackend::kDevice,
                     OpRegistrationPriority::kTop, OpEngine::kAiCore);

TEST(CustomOpRegistryPriorityTest, TopEngineRegistrationVisibleToGuardLookups) {
  // kTop下任意引擎注册对守卫类查询（显式kTop）可见
  const AscendString op_type("CustomOpRegistryTopEngineVisibleOp");
  EXPECT_TRUE(CustomOpFactory::IsExistOp(op_type, OpRegistrationPriority::kTop));
  EXPECT_TRUE(CustomOpFactory::IsExistOp(op_type, OpBackend::kDevice, OpRegistrationPriority::kTop));
  EXPECT_TRUE(CustomOpFactory::IsExistOp(op_type));
}

TEST(CustomOpRegistryPriorityTest, CollectionWhitelistCoversOnlyDefaultAndHcclBottomForms) {
  CustomOpRegistry registry;
  const AscendString top_custom("CustomOpRegistryCollectWhitelistTopCustomOp");
  const AscendString top_hccl("CustomOpRegistryCollectWhitelistTopHcclOp");
  const AscendString bottom_hccl("CustomOpRegistryCollectWhitelistBottomHcclOp");
  const AscendString bottom_aicore("CustomOpRegistryCollectWhitelistBottomAiCoreOp");
  const AscendString bottom_hostcpu("CustomOpRegistryCollectWhitelistBottomHostCpuOp");
  RegisterOp(registry, top_custom, OpBackend::kDevice, OpRegistrationPriority::kTop, OpEngine::kCustom, MakePriorityOp);
  RegisterOp(registry, top_hccl, OpBackend::kDevice, OpRegistrationPriority::kTop, OpEngine::kHccl, MakePriorityOp);
  RegisterOp(registry, bottom_hccl, OpBackend::kDevice, OpRegistrationPriority::kBottom, OpEngine::kHccl,
             MakePriorityOp);
  RegisterOp(registry, bottom_aicore, OpBackend::kDevice, OpRegistrationPriority::kBottom, OpEngine::kAiCore,
             MakePriorityOp);
  RegisterOp(registry, bottom_hostcpu, OpBackend::kHostCPU, OpRegistrationPriority::kBottom, OpEngine::kHostCpu,
             MakePriorityOp);
  // 白名单内：改前默认槽位(kTop + kCustom) 与 本次支持形态(kBottom + kHccl)
  EXPECT_TRUE(registry.IsCustomOpToBeCollected(top_custom));
  EXPECT_TRUE(registry.IsCustomOpToBeCollected(top_hccl));
  EXPECT_TRUE(registry.IsCustomOpToBeCollected(bottom_hccl));
  // 白名单外：非自定义引擎注册、框架内置(kBottom, kHostCpu)常量折叠通道
  EXPECT_FALSE(registry.IsCustomOpToBeCollected(bottom_aicore));
  EXPECT_FALSE(registry.IsCustomOpToBeCollected(bottom_hostcpu));
  EXPECT_FALSE(registry.IsCustomOpToBeCollected(AscendString("CustomOpRegistryCollectWhitelistNoSuchOp")));
}

TEST(CustomOpRegistryPriorityTest, GetArgsRefreshStrategyResolvesHcclRegisteredArgsUpdater) {
  // F5修复：kHccl + kBottom兜底注册的ArgsUpdater此前因发现路径写死kCustom/kTop被识别为kNone
  EXPECT_EQ(CustomOpFactory::GetArgsRefreshStrategy(AscendString("CustomOpRegistryHcclArgsUpdaterOp")),
            ArgsRefreshStrategy::kUpdateCallback);
}

TEST(CustomOpRegistryPriorityTest, CreateOrGetCustomOpWithResolvedEngineFindsHcclOp) {
  // V1 Distribute / V2 FindCustomOpFunc的两步式查找：GetOpEngine解析引擎 + 三参按优先级回退取实例
  const AscendString op_type("CustomOpRegistryHcclArgsUpdaterOp");
  OpEngine engine = OpEngine::kCustom;
  ASSERT_TRUE(CustomOpFactory::GetOpEngine(op_type, OpBackend::kDevice, engine));
  auto *const instance = CustomOpFactory::CreateOrGetCustomOp(op_type, OpBackend::kDevice, engine);
  ASSERT_NE(instance, nullptr);
  // 实例按 (priority, engine, backend) 缓存复用
  EXPECT_EQ(instance, CustomOpFactory::CreateOrGetCustomOp(op_type, OpBackend::kDevice, engine));
}

TEST(CustomOpRegistryPriorityTest, CreateOrGetCustomOpFallsBackToBottomPriority) {
  CustomOpRegistry registry;
  const OpEngine engine = OpEngine::kHccl;
  // 仅kBottom注册：三参按指定引擎逐优先级回退命中；指定未注册引擎返回空
  const AscendString bottom_only("CustomOpRegistryBottomOnlyResolveOp");
  RegisterOp(registry, bottom_only, OpBackend::kDevice, OpRegistrationPriority::kBottom, engine, MakePriorityOp);
  EXPECT_NE(registry.CreateOrGetCustomOp(bottom_only, OpBackend::kDevice, engine), nullptr);
  EXPECT_EQ(registry.CreateOrGetCustomOp(bottom_only, OpBackend::kDevice, OpEngine::kAiCore), nullptr);

  // kTop与kBottom同引擎注册：kTop优先
  const AscendString both("CustomOpRegistryTopPreferredResolveOp");
  RegisterOp(registry, both, OpBackend::kDevice, OpRegistrationPriority::kBottom, engine, MakePriorityOp);
  RegisterOp(registry, both, OpBackend::kDevice, OpRegistrationPriority::kTop, engine, MakeOtherOp);
  auto *const top_op = registry.CreateOrGetCustomOp(both, OpBackend::kDevice, OpRegistrationPriority::kTop, engine);
  ASSERT_NE(top_op, nullptr);
  EXPECT_EQ(registry.CreateOrGetCustomOp(both, OpBackend::kDevice, engine), top_op);
}

TEST(CustomOpRegistryPriorityTest, HcclOpTopRegistrationTakesPrecedenceOverBottom) {
  CustomOpRegistry registry;
  const OpEngine engine = OpEngine::kHccl;
  // 同一kHccl形态算子kTop/kBottom同时注册：引擎与实例解析均命中kTop，kBottom仅显式查询可达
  const AscendString op_type("CustomOpRegistryHcclTopBottomCoexistOp");
  RegisterOp(registry, op_type, OpBackend::kDevice, OpRegistrationPriority::kBottom, engine, MakePriorityOp);
  RegisterOp(registry, op_type, OpBackend::kDevice, OpRegistrationPriority::kTop, engine, MakeOtherOp);
  OpEngine resolved_engine = OpEngine::kCustom;
  ASSERT_TRUE(registry.GetOpEngine(op_type, OpBackend::kDevice, resolved_engine));
  EXPECT_EQ(resolved_engine, engine);
  auto *const top_op = registry.CreateOrGetCustomOp(op_type, OpBackend::kDevice, OpRegistrationPriority::kTop, engine);
  ASSERT_NE(top_op, nullptr);
  EXPECT_EQ(registry.CreateOrGetCustomOp(op_type, OpBackend::kDevice, engine), top_op);
  auto *const bottom_op =
      registry.CreateOrGetCustomOp(op_type, OpBackend::kDevice, OpRegistrationPriority::kBottom, engine);
  ASSERT_NE(bottom_op, nullptr);
  EXPECT_NE(bottom_op, top_op);
  // kTop存在时守卫类查询（带优先级形参）可见
  EXPECT_TRUE(registry.HasCreator(op_type, OpRegistrationPriority::kTop));
  EXPECT_TRUE(registry.HasCreator(op_type, OpBackend::kDevice, OpRegistrationPriority::kTop));
  EXPECT_TRUE(registry.HasCreator(op_type));

  // kTop生效语义：kBottom注册的ArgsUpdater能力被kTop注册遮蔽，刷新策略识别为kNone
  const AscendString shadow_op("CustomOpRegistryHcclTopShadowsBottomOp");
  RegisterOp(registry, shadow_op, OpBackend::kDevice, OpRegistrationPriority::kBottom, engine, MakeHcclArgsUpdaterOp);
  RegisterOp(registry, shadow_op, OpBackend::kDevice, OpRegistrationPriority::kTop, engine, MakePriorityOp);
  EXPECT_EQ(registry.GetArgsRefreshStrategy(shadow_op), ArgsRefreshStrategy::kNone);
}

TEST(CustomOpRegistryPriorityTest, GetAllRegisteredOpsIncludesBottomPriority) {
  CustomOpRegistry registry;
  const AscendString top_op("CustomOpRegistryListTopOp");
  const AscendString bottom_op("CustomOpRegistryListBottomOp");
  RegisterOp(registry, top_op, OpBackend::kDevice, OpRegistrationPriority::kTop, OpEngine::kCustom, MakePriorityOp);
  RegisterOp(registry, bottom_op, OpBackend::kDevice, OpRegistrationPriority::kBottom, OpEngine::kHccl, MakePriorityOp);
  std::vector<AscendString> all_registered_ops;
  ASSERT_EQ(registry.GetAllRegisteredOps(all_registered_ops), GRAPH_SUCCESS);
  EXPECT_NE(std::find(all_registered_ops.begin(), all_registered_ops.end(), top_op), all_registered_ops.end());
  EXPECT_NE(std::find(all_registered_ops.begin(), all_registered_ops.end(), bottom_op), all_registered_ops.end());
}

TEST(CustomOpRegistryPriorityTest, CommonCapabilityFallsBackToBottomPriority) {
  CustomOpRegistry registry;
  // kTop注册无ShapeInfer能力、kBottom注册提供：双参查询逐级回退命中kBottom
  const AscendString fallback_op("CustomOpRegistryCapabilityFallbackOp");
  RegisterOp(registry, fallback_op, OpBackend::kDevice, OpRegistrationPriority::kTop, OpEngine::kCustom,
             MakePriorityOp);
  RegisterOp(registry, fallback_op, OpBackend::kDevice, OpRegistrationPriority::kBottom, OpEngine::kAiCore,
             MakeShapeInferOp);
  EXPECT_NE(registry.GetCustomOpCommonCapability(fallback_op, CustomOpCapability::kShapeInfer), nullptr);
  // 显式kTop查询保持仅看kTop
  EXPECT_EQ(
      registry.GetCustomOpCommonCapability(fallback_op, CustomOpCapability::kShapeInfer, OpRegistrationPriority::kTop),
      nullptr);

  // kTop提供能力时不回退到kBottom
  const AscendString top_provider_op("CustomOpRegistryCapabilityTopPreferredOp");
  RegisterOp(registry, top_provider_op, OpBackend::kDevice, OpRegistrationPriority::kTop, OpEngine::kAiCore,
             MakeShapeInferOp);
  RegisterOp(registry, top_provider_op, OpBackend::kDevice, OpRegistrationPriority::kBottom, OpEngine::kVectorCore,
             MakeOtherShapeInferOp);
  auto *const top_provider = registry.GetCustomOpCommonCapability(top_provider_op, CustomOpCapability::kShapeInfer,
                                                                  OpRegistrationPriority::kTop);
  ASSERT_NE(top_provider, nullptr);
  EXPECT_EQ(registry.GetCustomOpCommonCapability(top_provider_op, CustomOpCapability::kShapeInfer), top_provider);
}
}  // namespace ge
