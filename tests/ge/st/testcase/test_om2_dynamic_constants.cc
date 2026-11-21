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
#include <array>
#include "securec.h"
#include <string>
#include <vector>
#include "acl/acl_rt.h"
#include "common/om2/dynamic_codegen/info_collect/kernel_code_builder.h"
#include "common/om2/dynamic_codegen/info_collect/kernel/common_collector_utils.h"
#include "ge/eager_style_graph_builder/cpp/es_graph_builder.h"
#include "engine/aicore/kernel/aclnn_op_execute_kernel.h"
#include "engine/aicore/launch_kernel/ai_core_launch_kernel.h"
#include "exe_graph/lowering/exe_graph_attrs.h"
#include "exe_graph/runtime/continuous_vector.h"
#include "exe_graph/runtime/gert_tensor_data.h"
#include "exe_graph/runtime/kernel_context.h"
#include "exe_graph/runtime/runtime_tensor.h"
#include "framework/common/gert_model_data_deserialize.h"
#include "framework/common/gert_model_data_serialize.h"
#include "framework/common/helper/om2_package_helper.h"
#include "graph/fast_graph/gert_graph_attributes.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/execute_graph_utils.h"
#include "graph/utils/graph_utils_ex.h"
#include "kernel/tensor_attr.h"
#include "register/kernel_registry.h"
#include "stub/gert_runtime_stub.h"
#include "../../common/om2_dynamic_host_graph.h"
#include "../../common/om2_generated_model_runner.h"
#include "../../ut/ge/graph/load/om2_package_test_utils.h"

namespace gert {
namespace {
using Om2DynamicConstantsST = ge::om2_test::DynamicPackageTest;
using ValueType = om2::Om2ValueType;
struct ConstantData {
  const char *name;
  ValueType type;
  std::vector<uint8_t> bytes;
};

template <typename T>
std::vector<uint8_t> ObjectBytes(const T &object) {
  const auto *bytes = reinterpret_cast<const uint8_t *>(&object);
  return {bytes, bytes + sizeof(object)};
}

template <typename T>
std::vector<uint8_t> VectorBytes(const T first, const T second) {
  auto storage = ContinuousVector::Create<T>(4U);
  auto *vector = reinterpret_cast<TypedContinuousVector<T> *>(storage.get());
  (void)vector->SetSize(2U);
  vector->MutableData()[0] = first;
  vector->MutableData()[1] = second;
  return {storage.get(), storage.get() + sizeof(ContinuousVector) + 4U * sizeof(T)};
}

std::vector<uint8_t> NestedVectorBytes() {
  const size_t overhead = ContinuousVectorVector::GetOverHeadLength(2U);
  std::vector<uint8_t> bytes(overhead + 2U * sizeof(ContinuousVector) + 2U * sizeof(int32_t), 0U);
  auto *vectors = reinterpret_cast<ContinuousVectorVector *>(bytes.data());
  vectors->Init(2U);
  auto *first = vectors->Add<int32_t>(2U);
  auto *data = static_cast<int32_t *>(first->MutableData());
  data[0] = -29;
  data[1] = 47;
  (void)vectors->Add<int32_t>(0U);
  return bytes;
}

std::vector<uint8_t> LaunchAttributeBytes() {
  aclrtLaunchKernelAttr attrs[5]{};
  attrs[0].id = ACL_RT_LAUNCH_KERNEL_ATTR_SCHEM_MODE;
  attrs[0].value.schemMode = 7U;
  attrs[1].id = ACL_RT_LAUNCH_KERNEL_ATTR_DYN_UBUF_SIZE;
  attrs[1].value.dynUBufSize = 8192U;
  attrs[2].id = ACL_RT_LAUNCH_KERNEL_ATTR_ENGINE_TYPE;
  attrs[2].value.engineType = static_cast<decltype(attrs[2].value.engineType)>(1);
  attrs[3].id = ACL_RT_LAUNCH_KERNEL_ATTR_BLOCKDIM_OFFSET;
  attrs[3].value.blockDimOffset = 13U;
  attrs[4].id = ACL_RT_LAUNCH_KERNEL_ATTR_ENABLE_PROFILING;
  attrs[4].value.enableProfiling = 1U;
  return ObjectBytes(attrs);
}

void AppendStructuredConstants(std::vector<ConstantData> &values) {
  aclrtLaunchKernelCfg launch{};
  launch.numAttrs = 5U;
  const kernel::AclnnDeterministicConfig deterministic{true, 1, true, 3};
  const kernel::AclnnOriginalDeterministicConfig original{1, 2, {}};
  const kernel::MixCoreArgs mix{kernel::MixType::MIX_VECTOR_CORE, 24U, 16U, 8U};
  // The TensorData restoration contract is process-local and borrows this live test storage.
  static int64_t data[] = {31, -41};
  const TensorData tensor_data(data, nullptr, sizeof(data), kOnHost);
  const TensorData empty_data(nullptr, nullptr, 0U, kOnHost);
  const std::vector<ConstantData> structures{
      {"nested", ValueType::kGertContinuousVectorVectorInt32Ptr, NestedVectorBytes()},
      {"launch_attributes", ValueType::kAclrtLaunchKernelAttrPtr, LaunchAttributeBytes()},
      {"launch_config", ValueType::kAclrtLaunchKernelCfgPtr, ObjectBytes(launch)},
      {"deterministic", ValueType::kAclnnDeterministicConfigPtr, ObjectBytes(deterministic)},
      {"original_deterministic", ValueType::kAclnnOriginalDeterministicConfigPtr, ObjectBytes(original)},
      {"mix_core", ValueType::kGertMixCoreArgsPtr, ObjectBytes(mix)},
      {"tensor_data", ValueType::kGertTensorDataObjectPtr, ObjectBytes(tensor_data)},
      {"empty_tensor_data", ValueType::kGertTensorDataObjectPtr, ObjectBytes(empty_data)}};
  values.insert(values.end(), structures.begin(), structures.end());
}

std::vector<ConstantData> ConstantValues() {
  const kernel::BuildTensorAttr attr{kOnHost, ge::DT_INT64, {ge::FORMAT_ND, ge::FORMAT_NCHW, {}}};
  const StorageShape shape{{2, 3}, {6}};
  const Tensor tensor({{3, 2}, {6}}, {ge::FORMAT_ND, ge::FORMAT_NCHW, {}}, kOnHost, ge::DT_INT32, nullptr);
  std::vector<ConstantData> values{
      {"boolean", ValueType::kBool, ObjectBytes(true)},
      {"signed32", ValueType::kInt32T, ObjectBytes(int32_t{-37})},
      {"signed64", ValueType::kInt64T, ObjectBytes(int64_t{-5000000001LL})},
      {"unsigned32", ValueType::kUInt32T, ObjectBytes(uint32_t{4000000001U})},
      {"unsigned64", ValueType::kUInt64T, ObjectBytes(uint64_t{5000000002ULL})},
      {"narrow_unsigned64", ValueType::kUInt64T, ObjectBytes(uint32_t{71U})},
      {"size", ValueType::kSizeT, ObjectBytes(size_t{5000000003ULL})},
      {"narrow_size", ValueType::kSizeT, ObjectBytes(uint32_t{83U})},
      {"boolean_size", ValueType::kSizeT, ObjectBytes(true)},
      {"dtype", ValueType::kGeDataType, ObjectBytes(ge::DT_INT64)},
      {"placement", ValueType::kGertTensorPlacement, ObjectBytes(kOnHost)},
      {"text", ValueType::kCStringPtr, {'a', '"', '\\', '\n', 'z', 0U}},
      {"bytes", ValueType::kBytePtr, {0U, 17U, 128U, 255U}},
      {"vector64", ValueType::kTypedContinuousVectorInt64Ptr, VectorBytes<int64_t>(-19, 5000000004LL)},
      {"vector_size", ValueType::kTypedContinuousVectorSizeTPtr, VectorBytes<size_t>(23U, 5000000005ULL)},
      {"tensor_attr", ValueType::kGertBuildTensorAttrPtr, ObjectBytes(attr)},
      {"const_storage_shape_probe", ValueType::kGertStorageShapePtr, ObjectBytes(shape)},
      {"tensor", ValueType::kGertTensorPtr, ObjectBytes(tensor)},
      {"null_pointer", ValueType::kVoidPtr, ObjectBytes(static_cast<void *>(nullptr))}};
  AppendStructuredConstants(values);
  return values;
}

std::vector<int64_t> ReadScalarConstants(KernelContext *context) {
  return {context->GetInputValue<bool>(2U),
          context->GetInputValue<int32_t>(3U),
          context->GetInputValue<int64_t>(4U),
          context->GetInputValue<uint32_t>(5U),
          static_cast<int64_t>(context->GetInputValue<uint64_t>(6U)),
          static_cast<int64_t>(context->GetInputValue<uint64_t>(7U)),
          static_cast<int64_t>(context->GetInputValue<size_t>(8U)),
          static_cast<int64_t>(context->GetInputValue<size_t>(9U)),
          static_cast<int64_t>(context->GetInputValue<size_t>(10U)),
          context->GetInputValue<ge::DataType>(11U),
          context->GetInputValue<TensorPlacement>(12U)};
}

std::vector<int64_t> ReadObjectConstants(KernelContext *context) {
  const auto *ints = context->GetInputPointer<TypedContinuousVector<int64_t>>(15U);
  const auto *sizes = context->GetInputPointer<TypedContinuousVector<size_t>>(16U);
  const auto *attr = context->GetInputPointer<kernel::BuildTensorAttr>(17U);
  const auto *shape = context->GetInputPointer<StorageShape>(18U);
  const auto *tensor = context->GetInputPointer<Tensor>(19U);
  return {static_cast<int64_t>(ints->GetSize()),
          ints->GetData()[0],
          ints->GetData()[1],
          static_cast<int64_t>(sizes->GetSize()),
          static_cast<int64_t>(sizes->GetData()[0]),
          static_cast<int64_t>(sizes->GetData()[1]),
          attr->placement,
          attr->data_type,
          attr->storage_format.GetOriginFormat(),
          attr->storage_format.GetStorageFormat(),
          static_cast<int64_t>(shape->GetOriginShape().GetDimNum()),
          shape->GetOriginShape().GetDim(0),
          shape->GetOriginShape().GetDim(1),
          static_cast<int64_t>(shape->GetStorageShape().GetDimNum()),
          shape->GetStorageShape().GetDim(0),
          tensor->GetDataType(),
          tensor->GetPlacement(),
          tensor->GetOriginShape().GetDim(0),
          tensor->GetOriginShape().GetDim(1),
          tensor->GetStorageShape().GetDim(0),
          tensor->GetOriginFormat(),
          tensor->GetStorageFormat(),
          context->GetInputValue<void *>(20U) == nullptr};
}

std::vector<int64_t> ReadLaunchConstants(KernelContext *context) {
  const auto *attrs = context->GetInputPointer<aclrtLaunchKernelAttr>(22U);
  const auto *launch = context->GetInputPointer<aclrtLaunchKernelCfg>(23U);
  const auto *det = context->GetInputPointer<kernel::AclnnDeterministicConfig>(24U);
  const auto *original = context->GetInputPointer<kernel::AclnnOriginalDeterministicConfig>(25U);
  const auto *mix = context->GetInputPointer<kernel::MixCoreArgs>(26U);
  return {attrs[0].id,
          attrs[0].value.schemMode,
          attrs[1].id,
          attrs[1].value.dynUBufSize,
          attrs[2].id,
          attrs[2].value.engineType,
          attrs[3].id,
          attrs[3].value.blockDimOffset,
          attrs[4].id,
          attrs[4].value.enableProfiling,
          launch->attrs == nullptr,
          launch->numAttrs,
          det->has_op_deterministic,
          det->op_deterministic,
          det->has_op_deterministic_level,
          det->op_deterministic_level,
          original->deterministic,
          original->deterministic_level,
          static_cast<int64_t>(mix->mix_type),
          mix->all_core_num,
          mix->vec_core_num,
          mix->ai_core_num};
}

std::vector<int64_t> ReadContainerConstants(KernelContext *context) {
  const auto *nested = context->GetInputPointer<ContinuousVectorVector>(21U);
  const auto *first = static_cast<const int32_t *>(nested->Get(0U)->GetData());
  const auto *data = context->GetInputPointer<TensorData>(27U);
  const auto *empty = context->GetInputPointer<TensorData>(28U);
  const auto *payload = static_cast<const int64_t *>(data->GetAddr());
  return {static_cast<int64_t>(nested->GetSize()),
          static_cast<int64_t>(nested->Get(0U)->GetSize()),
          first[0],
          first[1],
          static_cast<int64_t>(nested->Get(1U)->GetSize()),
          static_cast<int64_t>(data->GetSize()),
          data->GetPlacement(),
          payload[0],
          payload[1],
          empty->GetAddr() == nullptr,
          static_cast<int64_t>(empty->GetSize()),
          empty->GetPlacement()};
}

// The substitute only exposes restored values. Collection, generated initializers and ABI execution are real.
ge::graphStatus Om2ReadRestoredConstants(KernelContext *context) {
  auto *output = context->GetInputValue<Tensor *>(0U);
  const auto *seed = context->GetInputPointer<Tensor>(1U);
  auto values = ReadScalarConstants(context);
  const auto objects = ReadObjectConstants(context);
  values.insert(values.end(), objects.begin(), objects.end());
  // Byte-sized GetInputPointer<T> reads inline storage, but these chains hold payload pointers.
  const auto *text = context->GetInputValue<const uint8_t *>(13U);
  const auto *bytes = context->GetInputValue<const uint8_t *>(14U);
  values.insert(values.end(), text, text + 6U);
  values.insert(values.end(), bytes, bytes + 4U);
  const auto launch = ReadLaunchConstants(context);
  const auto containers = ReadContainerConstants(context);
  values.insert(values.end(), launch.begin(), launch.end());
  values.insert(values.end(), containers.begin(), containers.end());
  values.push_back(seed->GetData<int64_t>()[0]);
  if (output == nullptr || output->GetSize() != values.size() * sizeof(int64_t)) {
    return ge::GRAPH_PARAM_INVALID;
  }
  return memcpy_s(output->GetData<int64_t>(), output->GetSize(), values.data(), output->GetSize()) == EOK
             ? ge::GRAPH_SUCCESS
             : ge::GRAPH_FAILED;
}
REGISTER_KERNEL(Om2ReadRestoredConstants).RunFunc(Om2ReadRestoredConstants);

class RestoredConstantsCollector final : public om2::KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const om2::KernelCollectContext &context, om2::KernelProgramInfo &info) override {
    const auto status = om2::InitKernelProgramInfo(context, om2::OutputCreatePolicy::kGraphBound, false, info);
    if (status != ge::GRAPH_SUCCESS) {
      return status;
    }
    std::vector<om2::KernelInputPortInfo> ports{{"output", ValueType::kGertTensorPtr},
                                                {"seed", ValueType::kGertTensorPtr}};
    for (const auto &constant : ConstantValues()) {
      ports.push_back({constant.name, constant.type});
    }
    return om2::SetInputPortInfos(context, ports, info);
  }
};
REGISTER_KERNEL_CODE_BUILDER("Om2ReadRestoredConstants", RestoredConstantsCollector);

const std::vector<int64_t> kExpectedConstants{1,
                                              -37,
                                              -5000000001LL,
                                              4000000001LL,
                                              5000000002LL,
                                              71,
                                              5000000003LL,
                                              83,
                                              1,
                                              ge::DT_INT64,
                                              kOnHost,
                                              2,
                                              -19,
                                              5000000004LL,
                                              2,
                                              23,
                                              5000000005LL,
                                              kOnHost,
                                              ge::DT_INT64,
                                              ge::FORMAT_ND,
                                              ge::FORMAT_NCHW,
                                              2,
                                              2,
                                              3,
                                              1,
                                              6,
                                              ge::DT_INT32,
                                              kOnHost,
                                              3,
                                              2,
                                              6,
                                              ge::FORMAT_ND,
                                              ge::FORMAT_NCHW,
                                              1,
                                              'a',
                                              '"',
                                              '\\',
                                              '\n',
                                              'z',
                                              0,
                                              0,
                                              17,
                                              128,
                                              255,
                                              ACL_RT_LAUNCH_KERNEL_ATTR_SCHEM_MODE,
                                              7,
                                              ACL_RT_LAUNCH_KERNEL_ATTR_DYN_UBUF_SIZE,
                                              8192,
                                              ACL_RT_LAUNCH_KERNEL_ATTR_ENGINE_TYPE,
                                              1,
                                              ACL_RT_LAUNCH_KERNEL_ATTR_BLOCKDIM_OFFSET,
                                              13,
                                              ACL_RT_LAUNCH_KERNEL_ATTR_ENABLE_PROFILING,
                                              1,
                                              1,
                                              5,
                                              1,
                                              1,
                                              1,
                                              3,
                                              1,
                                              2,
                                              static_cast<int64_t>(kernel::MixType::MIX_VECTOR_CORE),
                                              24,
                                              16,
                                              8,
                                              2,
                                              2,
                                              -29,
                                              47,
                                              0,
                                              16,
                                              kOnHost,
                                              31,
                                              -41,
                                              1,
                                              0,
                                              kTensorPlacementEnd};

std::vector<int64_t> ExpectedConstants(const int64_t seed) {
  auto expected = kExpectedConstants;
  expected.push_back(seed);
  return expected;
}

bool AddConstants(ge::ExecuteGraph &graph, ge::FastNode *consumer, const std::vector<ConstantData> &constants) {
  const ge::GeTensorDesc desc(ge::GeShape({1}), ge::FORMAT_ND, ge::DT_UINT8);
  for (size_t i = 0U; i < constants.size(); ++i) {
    const auto &constant = constants[i];
    auto *node = om2_test::AddNode(graph, constant.name, "Const", {}, {desc});
    if (node == nullptr ||
        !ge::AttrUtils::SetZeroCopyBytes(node->GetOpDescBarePtr(), kConstValue,
                                         ge::Buffer::CopyFrom(constant.bytes.data(), constant.bytes.size())) ||
        graph.AddEdge(node, 0, consumer, static_cast<int32_t>(i + 2U)) == nullptr) {
      return false;
    }
  }
  return true;
}

bool SetConstantGraphMetadata(ge::ExecuteGraph &graph, ge::FastNode *output, ge::GeTensorDesc input_desc,
                              ge::GeTensorDesc output_desc) {
  input_desc.SetName("seed");
  output_desc.SetName("restored_values");
  return ge::AttrUtils::SetListTensor(graph, attr::kModelInputs, {ge::GeTensor(input_desc)}) &&
         ge::AttrUtils::SetListTensor(graph, attr::kModelOutputs, {ge::GeTensor(output_desc)}) &&
         ge::ExecuteGraphUtils::SetGraphOutputs(&graph, {{output, 0}}) == ge::GRAPH_SUCCESS;
}

std::shared_ptr<ge::ExecuteGraph> ConstantsGraph(const std::vector<ConstantData> &constants, bool use_es = false) {
  ge::es::EsGraphBuilder builder("restored_constants", static_cast<ge::GraphKind>(1));
  std::vector<ge::GNode> es_constants;
  if (use_es) {
    for (const auto &constant : constants) {
      auto temporary = constant.bytes;
      auto holder = builder.CreateConstV2(temporary, {static_cast<int64_t>(temporary.size())}, ge::DT_UINT8);
      if (holder.GetProducer() == nullptr) {
        return nullptr;
      }
      es_constants.push_back(*holder.GetProducer());
      std::fill(temporary.begin(), temporary.end(), 0U);
    }
  }
  auto *es_graph = builder.GetCGraphBuilder()->GetGraph();
  auto graph = use_es ? ge::GraphUtilsEx::Graph2ExecuteGraph(*es_graph)
                      : std::make_shared<ge::ExecuteGraph>("restored_constants");
  if (graph == nullptr) {
    return nullptr;
  }
  const ge::GeTensorDesc input_desc(ge::GeShape({1}), ge::FORMAT_ND, ge::DT_INT64);
  const ge::GeTensorDesc output_desc(ge::GeShape({static_cast<int64_t>(ExpectedConstants(0).size())}), ge::FORMAT_ND,
                                     ge::DT_INT64);
  auto *input = om2_test::AddNode(*graph, "seed", "Data", {}, {input_desc});
  auto *output = om2_test::AddNode(*graph, "output", "OutputData", {}, {output_desc});
  auto *read = om2_test::AddNode(*graph, "read_constants", "Om2ReadRestoredConstants",
                                 std::vector<ge::GeTensorDesc>(constants.size() + 2U, input_desc), {});
  if (input == nullptr || output == nullptr || read == nullptr) {
    return nullptr;
  }
  if (use_es) {
    auto target = es_graph->FindNodeByName("read_constants");
    if (target == nullptr) {
      return nullptr;
    }
    for (size_t i = 0U; i < es_constants.size(); ++i) {
      if (es_graph->AddDataEdge(es_constants[i], 0, *target, static_cast<int32_t>(i + 2U)) != ge::GRAPH_SUCCESS) {
        return nullptr;
      }
    }
  } else if (!AddConstants(*graph, read, constants)) {
    return nullptr;
  }
  if (!ge::AttrUtils::SetInt(input->GetOpDescPtr(), attr::kIndex, 0) || graph->AddEdge(output, 0, read, 0) == nullptr ||
      graph->AddEdge(input, 0, read, 1) == nullptr ||
      !SetConstantGraphMetadata(*graph, output, input_desc, output_desc)) {
    return nullptr;
  }
  return graph;
}

void CheckRestoredConstants(ge::om2_test::GeneratedModelRunner &runner, int64_t seed) {
  const auto expected = ExpectedConstants(seed);
  std::vector<int64_t> actual(expected.size(), -999);
  const int64_t count = static_cast<int64_t>(actual.size());
  Tensor input({{1}, {1}}, {ge::FORMAT_ND, ge::FORMAT_ND, {}}, kOnHost, ge::DT_INT64, &seed);
  Tensor output({{count}, {count}}, {ge::FORMAT_ND, ge::FORMAT_ND, {}}, kOnHost, ge::DT_INT64, actual.data());
  Tensor *inputs[] = {&input};
  Tensor *outputs[] = {&output};
  GertModelRunConfig run;
  run.input_count = 1U;
  run.input_data = inputs;
  run.output_count = 1U;
  run.output_data = outputs;
  ASSERT_EQ(runner.Run(run), ACL_SUCCESS);
  for (size_t i = 0U; i < expected.size(); ++i) {
    EXPECT_EQ(actual[i], expected[i]) << "restored field index=" << i << ", seed=" << seed;
  }
}

/**
 * 用例描述：多类型 Const 经真实收集、生成、归档与加载后，执行 Kernel 读取恢复对象的实际字段。
 * 预置条件：仅注册读取常量的 Host Kernel 和类型契约；27 个常量合并为一个模型、只构建一次 SO。
 * 测试步骤：构图并打包、序列化往返、Load，使用不同 seed 连续 Run 两次，最后 Unload。
 * 预期结果：有符号/无符号值、窄标量扩展、字符串转义、二进制、向量、结构字段逐项保持，输出匹配参考。
 */
void CompileAndCheckRestoredConstants(bool use_es) {
  ge::GetThreadLocalContext().SetGraphOption(
      {{"ge.buildConfig", "make -s CXXFLAGS='-std=c++17 -fPIC -O0 -D_GLIBCXX_USE_CXX11_ABI=0'"}});
  auto graph = ConstantsGraph(ConstantValues(), use_es);
  ASSERT_NE(graph, nullptr);
  auto facade = ge::GraphUtilsEx::ExecuteGraph2Graph(graph);
  ASSERT_NE(facade, nullptr);
  GertModelData built;
  ASSERT_EQ(ge::Om2PackageHelper().BuildGertModelDataFromGraph(*facade, built), ge::SUCCESS);
  ge::ModelBufferData archive;
  ASSERT_EQ(SerializeGertModelData(built, archive, false), ge::SUCCESS);
  GertModelData restored;
  ASSERT_EQ(DeserializeGertModelData(static_cast<const uint8_t *>(archive.data.get()), archive.length, &restored),
            ge::SUCCESS);
  ASSERT_EQ(restored.models.size(), 1U);
  ASSERT_NE(restored.models[0]->runtime, nullptr);
  ge::om2_test::GeneratedModelRunner runner;
  ASSERT_TRUE(runner.Open(restored.models[0]->runtime->so_artifact)) << runner.Error();
  GertModelLoadConfig load;
  ASSERT_EQ(runner.Load(load), ACL_SUCCESS);
  CheckRestoredConstants(runner, 123);
  CheckRestoredConstants(runner, -456);
  EXPECT_EQ(runner.Unload(), ACL_SUCCESS);
}

TEST_F(Om2DynamicConstantsST, MultipleConstantTypesSurvivePackageRoundTripAndExecution) {
  // slow_test_limit = 3000
  CompileAndCheckRestoredConstants(false);
}

/**
 * 用例描述：ES 自动命名的 Tensor 常量使用与 Buffer 路径相同的恢复与执行断言。
 * 预置条件：每个源字节数组在构图结束前覆写，ES builder 在编译前析构。
 * 测试步骤：用 CreateConstV2 创建相同载荷，经公开构建入口生成模型并重复执行。
 * 预期结果：标量、零字节和复杂对象字段在两轮执行中匹配 Buffer 路径的参考值。
 */
TEST_F(Om2DynamicConstantsST, EsConstantsSurviveBuilderLifetimeAndPackageRoundTrip) {
  // slow_test_limit = 3000
  CompileAndCheckRestoredConstants(true);
}

ge::graphStatus Om2ReadBuildRefHost(KernelContext *context) {
  const auto *tensor = context->GetInputPointer<Tensor>(0U);
  auto *output = context->MutableInputPointer<Tensor>(1U);
  if (tensor == nullptr || output == nullptr || tensor->GetOriginShape() != Shape({1, 3}) ||
      tensor->GetStorageShape() != Shape({3}) || tensor->GetPlacement() != kOnHost ||
      tensor->GetDataType() != ge::DT_INT64 || tensor->GetSize() != 3U * sizeof(int64_t)) {
    return ge::GRAPH_FAILED;
  }
  return memcpy_s(output->GetData<int64_t>(), output->GetSize(), tensor->GetData<int64_t>(), tensor->GetSize()) == EOK
             ? ge::GRAPH_SUCCESS
             : ge::GRAPH_FAILED;
}
REGISTER_KERNEL(Om2ReadBuildRefHost).RunFunc(Om2ReadBuildRefHost);

class ReadBuildRefHostCollector final : public om2::KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const om2::KernelCollectContext &context, om2::KernelProgramInfo &info) override {
    const auto status = om2::InitKernelProgramInfo(context, om2::OutputCreatePolicy::kGraphBound, false, info);
    return status == ge::GRAPH_SUCCESS
               ? om2::SetInputPortInfos(
                     context, {{"tensor", ValueType::kGertTensorPtr}, {"output", ValueType::kGertTensorPtr}}, info)
               : status;
  }
};
REGISTER_KERNEL_CODE_BUILDER("Om2ReadBuildRefHost", ReadBuildRefHostCollector);

void AttachHostConstants(ge::es::EsGraphBuilder &builder, ge::Graph &facade, ge::ExecuteGraph &graph, ge::FastNode &ref,
                         ge::GNode &target, const std::array<std::vector<uint8_t>, 3U> &constants,
                         const ge::GeTensorDesc &tensor_desc, bool use_es) {
  for (size_t port = 0U; port < constants.size(); ++port) {
    const auto &bytes = constants[port];
    if (use_es) {
      auto holder = builder.CreateConstV2(bytes, {static_cast<int64_t>(bytes.size())}, ge::DT_UINT8);
      ASSERT_NE(holder.GetProducer(), nullptr);
      ASSERT_EQ(facade.AddDataEdge(*holder.GetProducer(), 0, target, static_cast<int32_t>(port)), ge::GRAPH_SUCCESS);
    } else {
      const auto name = "old_value_" + std::to_string(port);
      auto *node = om2_test::AddNode(graph, name.c_str(), "Const", {}, {tensor_desc});
      ASSERT_NE(node, nullptr);
      ASSERT_TRUE(ge::AttrUtils::SetZeroCopyBytes(node->GetOpDescBarePtr(), kConstValue,
                                                  ge::Buffer::CopyFrom(bytes.data(), bytes.size())));
      ASSERT_NE(graph.AddEdge(node, 0, &ref, static_cast<int32_t>(port)), nullptr);
    }
  }
}

void RunHostConstantGraph(ge::om2_test::GeneratedModelRunner &runner, std::array<int64_t, 3U> &result) {
  GertModelLoadConfig load;
  ASSERT_EQ(runner.Load(load), ACL_SUCCESS);
  Tensor result_tensor({{3}, {3}}, {ge::FORMAT_ND, ge::FORMAT_ND, {}}, kOnHost, ge::DT_INT64, result.data());
  result_tensor.SetSize(sizeof(result));
  Tensor *outputs[]{&result_tensor};
  GertModelRunConfig run;
  run.output_count = 1U;
  run.output_data = outputs;
  ASSERT_EQ(runner.Run(run), ACL_SUCCESS);
  ASSERT_EQ(runner.Unload(), ACL_SUCCESS);
}

void CheckHostConstantResults(const std::array<int64_t, 3U> &result, const std::array<int64_t, 3U> &host_values,
                              std::array<int64_t, 3U> &legacy_result, bool use_es) {
  EXPECT_EQ(result, host_values);
  if (use_es) {
    EXPECT_EQ(result, legacy_result);
  } else {
    legacy_result = result;
  }
}

/**
 * 用例描述：BuildRefTensor 从自动命名的 ES Const 恢复动态 shape 列表及 Host 数据。
 * 预置条件：注册只读 Host probe，使用真实 BuildRefTensor 和生成的 OM2 SO，ACL 调用使用桩。
 * 测试步骤：用 Buffer 和 ES Tensor 两种载体构图，生成、加载并执行 SO。
 * 预期结果：两条路径的 origin/storage shape、placement、dtype 和数据逐项一致。
 */
TEST_F(Om2DynamicConstantsST, BuildRefTensorRestoresHostShapeListFromEsConstants) {
  // slow_test_limit = 7000
  ge::GetThreadLocalContext().SetGraphOption(
      {{"ge.buildConfig", "make -s CXXFLAGS='-std=c++17 -fPIC -O0 -D_GLIBCXX_USE_CXX11_ABI=0'"}});
  std::array<int64_t, 3U> host_values{-1, 17, 29};
  GertTensorData host_data;
  ASSERT_EQ(host_data.MutableTensorData().SetAddr(host_values.data(), nullptr), ge::GRAPH_SUCCESS);
  host_data.SetPlacement(kOnHost);
  host_data.SetSize(sizeof(host_values));
  const std::array<std::vector<uint8_t>, 3U> constants{
      ObjectBytes(StorageShape{{1, 3}, {3}}), ObjectBytes(host_data),
      ObjectBytes(kernel::BuildTensorAttr{kOnHost, ge::DT_INT64, {ge::FORMAT_ND, ge::FORMAT_ND, {}}})};
  std::array<int64_t, 3U> legacy_result{};
  for (const bool use_es : {false, true}) {
    SCOPED_TRACE(use_es ? "ES Tensor" : "Buffer");
    ge::es::EsGraphBuilder builder("build_ref_host", static_cast<ge::GraphKind>(1));
    auto *facade = builder.GetCGraphBuilder()->GetGraph();
    ASSERT_NE(facade, nullptr);
    auto graph = ge::GraphUtilsEx::Graph2ExecuteGraph(*facade);
    ASSERT_NE(graph, nullptr);
    const ge::GeTensorDesc tensor_desc(ge::GeShape({3}), ge::FORMAT_ND, ge::DT_INT64);
    auto *ref = om2_test::AddNode(*graph, "build_ref", "BuildRefTensor", {tensor_desc, tensor_desc, tensor_desc},
                                  {tensor_desc});
    auto *output = om2_test::AddNode(*graph, "output", "OutputData", {}, {tensor_desc});
    auto *probe = om2_test::AddNode(*graph, "read_host", "Om2ReadBuildRefHost", {tensor_desc, tensor_desc}, {});
    ASSERT_NE(ref, nullptr);
    ASSERT_NE(output, nullptr);
    ASSERT_NE(probe, nullptr);
    auto target = facade->FindNodeByName("build_ref");
    ASSERT_NE(target, nullptr);
    ASSERT_NO_FATAL_FAILURE(
        AttachHostConstants(builder, *facade, *graph, *ref, *target, constants, tensor_desc, use_es));
    ASSERT_NE(graph->AddEdge(ref, 0, probe, 0), nullptr);
    ASSERT_NE(graph->AddEdge(output, 0, probe, 1), nullptr);
    ASSERT_TRUE(ge::AttrUtils::SetListTensor(*graph, attr::kModelInputs, std::vector<ge::GeTensor>{}));
    ge::GeTensor output_tensor(tensor_desc);
    output_tensor.MutableTensorDesc().SetName("host_shape_list");
    ASSERT_TRUE(ge::AttrUtils::SetListTensor(*graph, attr::kModelOutputs, {output_tensor}));
    ASSERT_EQ(ge::ExecuteGraphUtils::SetGraphOutputs(graph.get(), {{output, 0}}), ge::GRAPH_SUCCESS);

    GertModelData model;
    ASSERT_EQ(ge::Om2PackageHelper().BuildGertModelDataFromGraph(*facade, model), ge::SUCCESS);
    ASSERT_EQ(model.models.size(), 1U);
    ASSERT_NE(model.models[0]->runtime, nullptr);
    ge::om2_test::GeneratedModelRunner runner;
    ASSERT_TRUE(runner.Open(model.models[0]->runtime->so_artifact)) << runner.Error();
    std::array<int64_t, 3U> result{};
    ASSERT_NO_FATAL_FAILURE(RunHostConstantGraph(runner, result));
    CheckHostConstantResults(result, host_values, legacy_result, use_es);
  }
}

struct InvalidConstant {
  size_t index;
  std::vector<uint8_t> bytes;
  const char *message;
};

void CheckRejectedConstant(const InvalidConstant &invalid) {
  auto constants = ConstantValues();
  SCOPED_TRACE(constants[invalid.index].name);
  constants[invalid.index].bytes = invalid.bytes;
  auto graph = ConstantsGraph(constants);
  ASSERT_NE(graph, nullptr);
  auto facade = ge::GraphUtilsEx::ExecuteGraph2Graph(graph);
  ASSERT_NE(facade, nullptr);
  GertRuntimeStub stub;
  GertModelData output;
  output.manifest = std::make_unique<GertModelDataManifest>();
  auto *original = output.manifest.get();
  output.manifest->model_num = 17U;
  stub.GetSlogStub().Clear();
  EXPECT_NE(ge::Om2PackageHelper().BuildGertModelDataFromGraph(*facade, output), ge::SUCCESS);
  EXPECT_NE(stub.GetSlogStub().FindLog(DLOG_ERROR, invalid.message), -1);
  EXPECT_EQ(output.manifest.get(), original);
  EXPECT_EQ(output.manifest->model_num, 17U);
  EXPECT_TRUE(output.models.empty());
}

/**
 * 用例描述：外部执行图的非法 Const 在收集阶段被拒绝，不发布半成品模型。
 * 预置条件：沿用真实打包流水线及合法多类型图，仅替换表中一个常量的原始 payload。
 * 测试步骤：依次注入标量截断、字符串未终止/内嵌零、向量布局错误及 shape 维数越界，调用打包入口。
 * 预期结果：每项得到对应语义错误日志，预置 manifest 保持不变且 models 为空；不进入 SO 构建。
 */
TEST_F(Om2DynamicConstantsST, InvalidConstantTableRejectsPackagingWithoutPublishingModel) {
  // A regression that reaches compilation must fail without compiling an unnecessary SO.
  ge::GetThreadLocalContext().SetGraphOption({{"ge.buildConfig", "false"}});
  auto invalid_vector = VectorBytes<int64_t>(1, 2);
  const size_t invalid_size = 5U;
  ASSERT_EQ(memcpy_s(invalid_vector.data() + sizeof(size_t), invalid_vector.size() - sizeof(size_t), &invalid_size,
                     sizeof(invalid_size)),
            EOK);
  auto short_vector = VectorBytes<int64_t>(1, 2);
  short_vector.pop_back();
  auto invalid_shape = ObjectBytes(StorageShape{{2, 3}, {6}});
  const size_t invalid_dims = Shape::kMaxDimNum + 1U;
  ASSERT_EQ(memcpy_s(invalid_shape.data(), invalid_shape.size(), &invalid_dims, sizeof(invalid_dims)), EOK);
  const kernel::MixCoreArgs invalid_mix{static_cast<kernel::MixType>(99), 24U, 16U, 8U};
  const std::vector<InvalidConstant> cases{
      {1U, {0U}, "OM2 Const node signed32 has 1 bytes"},
      {11U, {'a', 'b'}, "OM2 string Const node text is not null terminated"},
      {11U, {'a', 0U, 'b', 0U}, "OM2 string Const node text contains an embedded null"},
      {13U, {1U}, "OM2 ContinuousVector Const node vector64 is truncated"},
      {13U, invalid_vector, "OM2 ContinuousVector Const node vector64 has invalid size"},
      {13U, short_vector, "OM2 ContinuousVector Const node vector64 size mismatch"},
      {16U, invalid_shape, "OM2 StorageShape Const node const_storage_shape_probe has invalid dim count"},
      {24U, ObjectBytes(invalid_mix), "OM2 MixCoreArgs Const node mix_core has invalid mix type"}};
  for (const auto &invalid : cases) {
    CheckRejectedConstant(invalid);
  }
}
}  // namespace
}  // namespace gert
