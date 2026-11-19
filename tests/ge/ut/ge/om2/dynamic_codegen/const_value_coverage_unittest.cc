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
#include "securec.h"
#include <limits>
#include "acl/acl_rt.h"
#include "common/om2/dynamic_codegen/info_collect/const_value_collector.h"
#include "common/om2/dynamic_codegen/info_collect/kernel_code_builder.h"
#include "common/op_tiling/tiling_dfx.h"
#include "engine/aicore/kernel/aclnn_op_execute_kernel.h"
#include "engine/aicore/launch_kernel/ai_core_launch_kernel.h"
#include "engine/aicore/launch_kernel/rt_kernel_launch_args_ex.h"
#include "exe_graph/lowering/exe_graph_attrs.h"
#include "exe_graph/runtime/continuous_vector.h"
#include "exe_graph/runtime/gert_tensor_data.h"
#include "exe_graph/runtime/op_execute_context.h"
#include "exe_graph/runtime/runtime_tensor.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/tensor_utils.h"
#include "kernel/known_subgraph/davinci_model_kernel.h"
#include "kernel/tensor_attr.h"
#include "stage/stage_splitter.h"
#include "stub/gert_runtime_stub.h"

namespace gert {
namespace om2 {
namespace {
using Type = Om2ValueType;
using Bytes = std::vector<uint8_t>;

template <typename T>
Bytes ObjectBytes(const T &value) {
  const auto *begin = reinterpret_cast<const uint8_t *>(&value);
  return {begin, begin + sizeof(value)};
}

template <typename T>
Bytes VectorBytes(const std::vector<T> &items) {
  const size_t capacity = items.size() + 1U;
  auto storage = ContinuousVector::Create<T>(capacity);
  auto *vector = reinterpret_cast<TypedContinuousVector<T> *>(storage.get());
  EXPECT_EQ(vector->SetSize(items.size()), ge::GRAPH_SUCCESS);
  std::copy(items.begin(), items.end(), vector->MutableData());
  return {storage.get(), storage.get() + sizeof(ContinuousVector) + capacity * sizeof(T)};
}

template <typename T>
Bytes Replace(Bytes bytes, const size_t offset, const T &value) {
  if (offset > bytes.size()) {
    ADD_FAILURE() << "Payload offset exceeds buffer size";
    return bytes;
  }
  EXPECT_EQ(memcpy_s(bytes.data() + offset, bytes.size() - offset, &value, sizeof(value)), EOK);
  return bytes;
}

class ConstantConsumer final : public KernelCodeBuilder {
 public:
  ge::graphStatus Collect(const KernelCollectContext &context, KernelProgramInfo &info) override {
    int64_t type = 0;
    if (!ge::AttrUtils::GetInt(context.node.GetOpDescBarePtr(), "test_value_type", type)) {
      return ge::GRAPH_PARAM_INVALID;
    }
    info.execution_node_name = context.node.GetNamePtr();
    info.inputs = {{context.input_chain_indices.at(0U), "constant", static_cast<Type>(type)}};
    return ge::GRAPH_SUCCESS;
  }
};
REGISTER_KERNEL_CODE_BUILDER("Om2ConstantCoverageConsumer", ConstantConsumer);

class ConstantGraph {
 public:
  void Prepare(const Type type, const Bytes &bytes, const char *name = "value") {
    auto producer = std::make_shared<ge::OpDesc>(name, "Const");
    ASSERT_EQ(producer->AddOutputDesc(ge::GeTensorDesc()), ge::GRAPH_SUCCESS);
    ASSERT_TRUE(
        ge::AttrUtils::SetZeroCopyBytes(producer, kConstValue, ge::Buffer::CopyFrom(bytes.data(), bytes.size())));
    constant = graph.AddNode(producer);
    auto consumer = std::make_shared<ge::OpDesc>("consumer", "Om2ConstantCoverageConsumer");
    ASSERT_EQ(consumer->AddInputDesc(ge::GeTensorDesc()), ge::GRAPH_SUCCESS);
    ASSERT_TRUE(ge::AttrUtils::SetInt(consumer, "test_value_type", static_cast<int64_t>(type)));
    auto *node = graph.AddNode(consumer);
    ASSERT_NE(constant, nullptr);
    ASSERT_NE(node, nullptr);
    ASSERT_NE(graph.AddEdge(constant, 0, node, 0), nullptr);
    ge::StagePlan plan;
    ASSERT_EQ(ge::StageAnalyzer(graph).Analyze(plan), ge::GRAPH_SUCCESS);
    ASSERT_EQ(ge::StageSplitter(graph).Split(plan, {}), ge::GRAPH_SUCCESS);
    ASSERT_EQ(graph.TopologicalSorting(), ge::GRAPH_SUCCESS);
    ASSERT_EQ(normalizer.Normalize(graph, info.chains), ge::GRAPH_SUCCESS);
    std::vector<uint32_t> inputs, outputs;
    ASSERT_EQ(normalizer.GetInputChainIndices(*node, inputs), ge::GRAPH_SUCCESS);
    KernelCollectContext context(*node, inputs, outputs);
    auto builder = KernelCodeBuilderFactory::GetInstance().Create("Om2ConstantCoverageConsumer");
    ASSERT_NE(builder, nullptr);
    info.main.kernels.resize(1U);
    ASSERT_EQ(builder->Collect(context, info.main.kernels[0]), ge::GRAPH_SUCCESS);
  }
  ge::graphStatus Collect() {
    return ConstValueCollector().Collect(graph, normalizer, info);
  }
  uint32_t Index() const {
    return info.main.kernels.at(0).inputs.at(0).chain_index;
  }
  ge::ExecuteGraph graph{"constants"};
  ge::FastNode *constant = nullptr;
  ChainNormalizer normalizer;
  ProgramInfo info;
};

void ExpectInvalid(const Type type, const Bytes &bytes, const char *name = "value") {
  ConstantGraph fixture;
  ASSERT_NO_FATAL_FAILURE(fixture.Prepare(type, bytes, name));
  EXPECT_NE(fixture.Collect(), ge::GRAPH_SUCCESS);
  EXPECT_TRUE(fixture.info.const_chains.empty());
  EXPECT_TRUE(fixture.info.custom_const_chains.empty());
}

Om2ConstValue CollectValue(const Type type, const Bytes &bytes) {
  ConstantGraph fixture;
  fixture.Prepare(type, bytes);
  EXPECT_EQ(fixture.Collect(), ge::GRAPH_SUCCESS);
  EXPECT_EQ(fixture.info.const_chains.size(), 1U);
  if (fixture.info.const_chains.empty()) {
    return {};
  }
  EXPECT_EQ(fixture.info.const_chains[0].chain_index, fixture.Index());
  EXPECT_EQ(fixture.info.const_chains[0].value.type, type);
  EXPECT_EQ(fixture.info.const_chains[0].producer_name, "value");
  EXPECT_TRUE(fixture.info.custom_const_chains.empty());
  return fixture.info.const_chains[0].value;
}

TEST(Om2ConstValueCoverage, ScalarsPreserveSignednessWidthsAndBooleanConversions) {
  struct Case {
    Type type;
    Bytes bytes;
    uint64_t expected;
  };
  const std::vector<Case> cases{{Type::kBool, ObjectBytes(true), 1U},
                                {Type::kBool, ObjectBytes(false), 0U},
                                {Type::kUInt32T, ObjectBytes(uint32_t{4000000001U}), 4000000001U},
                                {Type::kUInt64T, ObjectBytes(uint64_t{5000000002ULL}), 5000000002ULL},
                                {Type::kUInt64T, ObjectBytes(uint32_t{71U}), 71U},
                                {Type::kSizeT, ObjectBytes(size_t{5000000003ULL}), 5000000003ULL},
                                {Type::kSizeT, ObjectBytes(uint32_t{83U}), 83U},
                                {Type::kSizeT, ObjectBytes(true), 1U},
                                {Type::kSizeT, ObjectBytes(false), 0U},
                                {Type::kGeDataType, ObjectBytes(ge::DT_INT64), ge::DT_INT64},
                                {Type::kGertTensorPlacement, ObjectBytes(kOnHost), kOnHost},
                                {Type::kGertCoreTypeIndexPtr, ObjectBytes(uint32_t{2U}), 2U},
                                {Type::kGeOppImplVersion, ObjectBytes(uint32_t{1U}), 1U}};
  for (const auto &item : cases) {
    SCOPED_TRACE(static_cast<int>(item.type));
    EXPECT_EQ(CollectValue(item.type, item.bytes).unsigned_value, item.expected);
    ExpectInvalid(item.type, Bytes{1U, 2U});
  }
  EXPECT_EQ(CollectValue(Type::kInt32T, ObjectBytes(int32_t{-37})).signed_value, -37);
  EXPECT_EQ(CollectValue(Type::kInt64T, ObjectBytes(int64_t{-5000000001LL})).signed_value, -5000000001LL);
  ExpectInvalid(Type::kInt32T, {1U});
  ExpectInvalid(Type::kInt64T, {1U});
}

TEST(Om2ConstValueCoverage, StringsAndRawBytesPreserveContentAndRejectEmbeddedNulls) {
  EXPECT_EQ(CollectValue(Type::kCStringPtr, {'a', '"', '\\', '\n', 0U}).string_value, "a\"\\\n");
  EXPECT_TRUE(CollectValue(Type::kCStringPtr, {0U}).string_value.empty());
  EXPECT_EQ(CollectValue(Type::kBytePtr, {0U, 17U, 128U, 255U}).raw_bytes, (Bytes{0U, 17U, 128U, 255U}));
  EXPECT_TRUE(CollectValue(Type::kBytePtr, {}).raw_bytes.empty());
  for (const auto &invalid : std::vector<Bytes>{{}, {'a'}, {'a', 0U, 'b', 0U}}) {
    ExpectInvalid(Type::kCStringPtr, invalid);
  }
}

TEST(Om2ConstValueCoverage, PortableVectorsCopyOnlySizeElementsAndValidateLayout) {
  EXPECT_EQ(CollectValue(Type::kTypedContinuousVectorInt64Ptr, VectorBytes<int64_t>({-19, 5000000004LL}))
                .continuous_vector_int64,
            (std::vector<int64_t>{-19, 5000000004LL}));
  EXPECT_EQ(CollectValue(Type::kTypedContinuousVectorSizeTPtr, VectorBytes<size_t>({23U, 5000000005ULL}))
                .continuous_vector_size_t,
            (std::vector<size_t>{23U, 5000000005ULL}));
  EXPECT_TRUE(
      CollectValue(Type::kTypedContinuousVectorInt64Ptr, VectorBytes<int64_t>({})).continuous_vector_int64.empty());
  EXPECT_TRUE(
      CollectValue(Type::kTypedContinuousVectorSizeTPtr, VectorBytes<size_t>({})).continuous_vector_size_t.empty());
  const auto valid = VectorBytes<int64_t>({1, 2});
  for (const auto type : {Type::kTypedContinuousVectorInt64Ptr, Type::kTypedContinuousVectorSizeTPtr}) {
    ExpectInvalid(type, {1U});
    ExpectInvalid(type, Replace(valid, sizeof(size_t), size_t{4U}));
    ExpectInvalid(type, Replace(valid, 0U, size_t{4U}));
    ExpectInvalid(type, Replace(valid, 0U, std::numeric_limits<size_t>::max()));
  }
}
Bytes NestedVectorBytes() {
  const size_t overhead = ContinuousVectorVector::GetOverHeadLength(2U);
  Bytes bytes(overhead + 2U * sizeof(ContinuousVector) + 2U * sizeof(int32_t), 0U);
  auto *vectors = reinterpret_cast<ContinuousVectorVector *>(bytes.data());
  vectors->Init(2U);
  auto *first = vectors->Add<int32_t>(2U);
  auto *data = static_cast<int32_t *>(first->MutableData());
  data[0] = -29;
  data[1] = 47;
  (void)vectors->Add<int32_t>(0U);
  return bytes;
}

TEST(Om2ConstValueCoverage, NestedVectorsPreserveEmptyChildrenAndRejectMalformedOffsets) {
  const auto type = Type::kGertContinuousVectorVectorInt32Ptr;
  const auto bytes = NestedVectorBytes();
  EXPECT_EQ(CollectValue(type, bytes).continuous_vector_vector_int32,
            (std::vector<std::vector<int32_t>>{{-29, 47}, {}}));
  EXPECT_TRUE(CollectValue(type, Bytes(sizeof(ContinuousVectorVector), 0U)).continuous_vector_vector_int32.empty());
  const auto overhead = ContinuousVectorVector::GetOverHeadLength(2U);
  ExpectInvalid(type, {0U});
  ExpectInvalid(type, Replace(bytes, sizeof(size_t), size_t{1U}));
  ExpectInvalid(type, Replace(bytes, sizeof(ContinuousVectorVector) - sizeof(size_t), overhead + 1U));
  ExpectInvalid(type, Replace(bytes, overhead + sizeof(size_t), size_t{3U}));
  ExpectInvalid(type, Replace(bytes, overhead, std::numeric_limits<size_t>::max()));
  auto extra_byte = bytes;
  extra_byte.push_back(0U);
  ExpectInvalid(type, extra_byte);
  auto truncated = bytes;
  truncated.resize(overhead);
  ExpectInvalid(type, truncated);
}

TEST(Om2ConstValueCoverage, ArgumentIndexVectorsRetainRolesAndRejectUnknownRole) {
  const auto type = Type::kTypedContinuousVectorArgsIndexToIoIndexPtr;
  const auto bytes = VectorBytes<optiling::ArgsIndexToIoIndex>(
      {{optiling::ArgsRole::kInput, 3U, 7U}, {optiling::ArgsRole::kOutput, 5U, 11U}});
  const auto value = CollectValue(type, bytes);
  ASSERT_EQ(value.continuous_vector_args_index_to_io_index.size(), 2U);
  const auto &input = value.continuous_vector_args_index_to_io_index[0];
  const auto &output = value.continuous_vector_args_index_to_io_index[1];
  EXPECT_EQ(input.args_role, static_cast<int32_t>(optiling::ArgsRole::kInput));
  EXPECT_EQ(input.args_index, 3U);
  EXPECT_EQ(input.io_index, 7U);
  EXPECT_EQ(output.args_role, static_cast<int32_t>(optiling::ArgsRole::kOutput));
  EXPECT_EQ(output.args_index, 5U);
  EXPECT_EQ(output.io_index, 11U);
  EXPECT_TRUE(CollectValue(type, VectorBytes<optiling::ArgsIndexToIoIndex>({}))
                  .continuous_vector_args_index_to_io_index.empty());
  ExpectInvalid(type, VectorBytes<optiling::ArgsIndexToIoIndex>({{static_cast<optiling::ArgsRole>(2), 0U, 0U}}));
}

Bytes LaunchAttributeBytes() {
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

TEST(Om2ConstValueCoverage, LaunchAttributesRequireCanonicalOrderAndDetachedConfig) {
  const auto bytes = LaunchAttributeBytes();
  const auto value = CollectValue(Type::kAclrtLaunchKernelAttrPtr, bytes);
  ASSERT_EQ(value.launch_kernel_attrs.size(), 5U);
  const std::vector<uint32_t> expected{7U, 8192U, 1U, 13U, 1U};
  const std::vector<aclrtLaunchKernelAttrId> ids{
      ACL_RT_LAUNCH_KERNEL_ATTR_SCHEM_MODE, ACL_RT_LAUNCH_KERNEL_ATTR_DYN_UBUF_SIZE,
      ACL_RT_LAUNCH_KERNEL_ATTR_ENGINE_TYPE, ACL_RT_LAUNCH_KERNEL_ATTR_BLOCKDIM_OFFSET,
      ACL_RT_LAUNCH_KERNEL_ATTR_ENABLE_PROFILING};
  for (size_t i = 0U; i < expected.size(); ++i) {
    EXPECT_EQ(value.launch_kernel_attrs[i].id, static_cast<int32_t>(ids[i]));
    EXPECT_EQ(value.launch_kernel_attrs[i].value, expected[i]);
  }
  ExpectInvalid(Type::kAclrtLaunchKernelAttrPtr, {0U});
  ExpectInvalid(Type::kAclrtLaunchKernelAttrPtr, Replace(bytes, 0U, ACL_RT_LAUNCH_KERNEL_ATTR_DYN_UBUF_SIZE));
  aclrtLaunchKernelCfg config{};
  config.numAttrs = 5U;
  EXPECT_EQ(CollectValue(Type::kAclrtLaunchKernelCfgPtr, ObjectBytes(config)).unsigned_value, 5U);
  aclrtLaunchKernelAttr attribute{};
  config.attrs = &attribute;
  ExpectInvalid(Type::kAclrtLaunchKernelCfgPtr, ObjectBytes(config));
  config.attrs = nullptr;
  config.numAttrs = 4U;
  ExpectInvalid(Type::kAclrtLaunchKernelCfgPtr, ObjectBytes(config));
}

TEST(Om2ConstValueCoverage, DeterminismAndMixCoreStructuresPreserveAllFields) {
  const kernel::AclnnDeterministicConfig deterministic{true, 1, true, 3};
  const auto det = CollectValue(Type::kAclnnDeterministicConfigPtr, ObjectBytes(deterministic));
  EXPECT_TRUE(det.aclnn_deterministic_config.has_op_deterministic);
  EXPECT_EQ(det.aclnn_deterministic_config.op_deterministic, 1);
  EXPECT_TRUE(det.aclnn_deterministic_config.has_op_deterministic_level);
  EXPECT_EQ(det.aclnn_deterministic_config.op_deterministic_level, 3);
  const kernel::AclnnOriginalDeterministicConfig original{1, 2, {}};
  const auto prior = CollectValue(Type::kAclnnOriginalDeterministicConfigPtr, ObjectBytes(original));
  EXPECT_EQ(prior.aclnn_original_deterministic_config.deterministic, 1);
  EXPECT_EQ(prior.aclnn_original_deterministic_config.deterministic_level, 2);
  for (const auto type : {kernel::MixType::MIX_AICORE, kernel::MixType::MIX_VECTOR_CORE}) {
    const kernel::MixCoreArgs mix{type, 24U, 16U, 8U};
    const auto value = CollectValue(Type::kGertMixCoreArgsPtr, ObjectBytes(mix));
    EXPECT_EQ(value.mix_core_args.mix_type, static_cast<uint32_t>(type));
    EXPECT_EQ(value.mix_core_args.all_core_num, 24U);
    EXPECT_EQ(value.mix_core_args.vec_core_num, 16U);
    EXPECT_EQ(value.mix_core_args.ai_core_num, 8U);
  }
  const kernel::MixCoreArgs invalid{static_cast<kernel::MixType>(99U), 0U, 0U, 0U};
  ExpectInvalid(Type::kGertMixCoreArgsPtr, ObjectBytes(invalid));
}

TEST(Om2ConstValueCoverage, TensorAttributesDfxAndExecutionOptionsCopySemanticFields) {
  const kernel::BuildTensorAttr attr{kOnHost, ge::DT_INT64, {ge::FORMAT_ND, ge::FORMAT_NCHW, {}}};
  const auto tensor = CollectValue(Type::kGertBuildTensorAttrPtr, ObjectBytes(attr));
  EXPECT_EQ(tensor.build_tensor_attr.placement, kOnHost);
  EXPECT_EQ(tensor.build_tensor_attr.data_type, ge::DT_INT64);
  EXPECT_EQ(tensor.build_tensor_attr.origin_format, ge::FORMAT_ND);
  EXPECT_EQ(tensor.build_tensor_attr.storage_format, ge::FORMAT_NCHW);
  const DfxExeArg dfx{true, false, 8192};
  const auto value = CollectValue(Type::kGertDfxExeArgPtr, ObjectBytes(dfx));
  EXPECT_TRUE(value.dfx_exe_arg.need_assert);
  EXPECT_FALSE(value.dfx_exe_arg.need_print);
  EXPECT_EQ(value.dfx_exe_arg.buffer_size, 8192);
  const OpExecuteOptions options{};
  EXPECT_EQ(CollectValue(Type::kGertOpExecuteOptionsPtr, ObjectBytes(options)).op_execute_options,
            ObjectBytes(options));
  const rtTaskCfgInfo_t config{};
  EXPECT_EQ(CollectValue(Type::kRtTaskCfgInfoTPtr, ObjectBytes(config)).type, Type::kRtTaskCfgInfoTPtr);
  ExpectInvalid(Type::kRtTaskCfgInfoTPtr, Replace(ObjectBytes(config), 0U, uint8_t{1U}));
  for (const auto type : {Type::kGertBuildTensorAttrPtr, Type::kGertDfxExeArgPtr, Type::kGertOpExecuteOptionsPtr,
                          Type::kRtTaskCfgInfoTPtr, Type::kAclrtLaunchKernelCfgPtr, Type::kAclnnDeterministicConfigPtr,
                          Type::kAclnnOriginalDeterministicConfigPtr, Type::kGertMixCoreArgsPtr}) {
    ExpectInvalid(type, {0U});
  }
}

TEST(Om2ConstValueCoverage, ComputeNodeDescriptorCopiesTrailingCompiledArgumentsAndValidatesSize) {
  using Desc = RtKernelLaunchArgsEx::ComputeNodeDesc;
  Desc source{};
  source.input_num = 2U;
  source.output_num = 3U;
  source.workspace_cap = 4U;
  source.max_tiling_data = 1024U;
  source.need_shape_buffer = true;
  source.need_overflow = true;
  source.compiled_args_size = 3U;
  auto bytes = ObjectBytes(source);
  bytes.resize(sizeof(source) + 3U);
  const Bytes args{7U, 0U, 255U};
  std::copy(args.begin(), args.end(), bytes.begin() + offsetof(Desc, compiled_args));
  const auto value = CollectValue(Type::kRtKernelLaunchArgsExComputeNodeDescPtr, bytes).compute_node_desc;
  EXPECT_EQ(value.input_num, 2U);
  EXPECT_EQ(value.output_num, 3U);
  EXPECT_EQ(value.workspace_cap, 4U);
  EXPECT_EQ(value.max_tiling_data, 1024U);
  EXPECT_TRUE(value.need_shape_buffer);
  EXPECT_TRUE(value.need_overflow);
  EXPECT_EQ(value.compiled_args, args);
  ExpectInvalid(Type::kRtKernelLaunchArgsExComputeNodeDescPtr, {0U});
  ExpectInvalid(Type::kRtKernelLaunchArgsExComputeNodeDescPtr, ObjectBytes(source));
  source.compiled_args_size = std::numeric_limits<size_t>::max();
  ExpectInvalid(Type::kRtKernelLaunchArgsExComputeNodeDescPtr, ObjectBytes(source));
}

Bytes ArgsDescriptorBytes() {
  Bytes bytes(sizeof(ArgsInfosDesc) + sizeof(ArgsInfosDesc::ArgsInfo), 0U);
  auto *desc = reinterpret_cast<ArgsInfosDesc *>(bytes.data());
  desc->Init(1U, 0U, 1U, 0U);
  auto *arg = static_cast<ArgsInfosDesc::ArgsInfo *>(desc->MutableArgsInfoBase());
  arg->Init(ArgsInfosDesc::ArgsInfo::ArgsInfoType::INPUT, ArgsInfosDesc::ArgsInfo::ArgsInfoFormat::FOLDED_DESC_ADDR, 2,
            8U);
  arg->folded_first = true;
  arg->be_folded = true;
  arg->dyn_desc = {true, 3U, 5U, 7U, 11U, 13U, 17U};
  arg->data_size = 19U;
  return bytes;
}

TEST(Om2ConstValueCoverage, ArgsDescriptorPreservesDynamicFieldsAndRejectsTruncation) {
  const auto type = Type::kGertArgsInfosDescPtr;
  const auto bytes = ArgsDescriptorBytes();
  const auto value = CollectValue(type, bytes).args_infos_desc;
  EXPECT_EQ(value.input_args_info_num, 1U);
  EXPECT_EQ(value.output_args_info_num, 0U);
  EXPECT_EQ(value.input_valid_num, 1U);
  EXPECT_EQ(value.output_valid_num, 0U);
  ASSERT_EQ(value.args_infos.size(), 1U);
  const auto &arg = value.args_infos[0];
  EXPECT_EQ(arg.arg_type, static_cast<int32_t>(ArgsInfosDesc::ArgsInfo::ArgsInfoType::INPUT));
  EXPECT_EQ(arg.arg_format, static_cast<int32_t>(ArgsInfosDesc::ArgsInfo::ArgsInfoFormat::FOLDED_DESC_ADDR));
  EXPECT_EQ(arg.start_index, 2);
  EXPECT_EQ(arg.arg_size, 8U);
  EXPECT_TRUE(arg.folded_first);
  EXPECT_TRUE(arg.be_folded);
  EXPECT_TRUE(arg.dyn_desc.is_group_first);
  EXPECT_EQ(arg.dyn_desc.io_group_id, 3U);
  EXPECT_EQ(arg.dyn_desc.io_index, 5U);
  EXPECT_EQ(arg.dyn_desc.dyn_num, 7U);
  EXPECT_EQ(arg.dyn_desc.group_size, 11U);
  EXPECT_EQ(arg.dyn_desc.group_offset, 13U);
  EXPECT_EQ(arg.dyn_desc.ptr_offset, 17U);
  EXPECT_EQ(arg.data_size, 19U);
  ExpectInvalid(type, {0U});
  auto truncated = bytes;
  truncated.pop_back();
  ExpectInvalid(type, truncated);
  ExpectInvalid(type, Replace(bytes, 0U, std::numeric_limits<size_t>::max()));
}

TEST(Om2ConstValueCoverage, MemoryOffsetsAndFileConstantVectorsPreserveEntriesAndRejectInvalidTags) {
  const kernel::MemoryBaseTypeOffset offsets[] = {{kernel::MemoryBaseType::kMemoryBaseTypeWeight, -8, 16U},
                                                  {kernel::MemoryBaseType::kMemoryBaseTypeFileConstant, 32, 64U}};
  const auto value = CollectValue(Type::kMemoryBaseTypeOffsetPtr, ObjectBytes(offsets));
  ASSERT_EQ(value.memory_base_type_offsets.size(), 2U);
  EXPECT_EQ(value.memory_base_type_offsets[0].base_type, 0U);
  EXPECT_EQ(value.memory_base_type_offsets[0].offset, -8);
  EXPECT_EQ(value.memory_base_type_offsets[0].size, 16U);
  EXPECT_EQ(value.memory_base_type_offsets[1].base_type, 1U);
  EXPECT_EQ(value.memory_base_type_offsets[1].offset, 32);
  EXPECT_EQ(value.memory_base_type_offsets[1].size, 64U);
  ExpectInvalid(Type::kMemoryBaseTypeOffsetPtr, {});
  ExpectInvalid(Type::kMemoryBaseTypeOffsetPtr, {0U});
  ExpectInvalid(Type::kMemoryBaseTypeOffsetPtr,
                Replace(ObjectBytes(offsets), 0U, kernel::MemoryBaseType::kMemoryBaseTypeEnd));
  const kernel::FileConstantNameAndMem file{"weights.bin", nullptr, 128U};
  const auto files = CollectValue(Type::kContinuousVectorFileConstantNameAndMemPtr,
                                  VectorBytes<kernel::FileConstantNameAndMem>({file}));
  ASSERT_EQ(files.file_constant_name_and_mems.size(), 1U);
  EXPECT_EQ(files.file_constant_name_and_mems[0].name, "weights.bin");
  EXPECT_EQ(files.file_constant_name_and_mems[0].mem, 0U);
  EXPECT_EQ(files.file_constant_name_and_mems[0].size, 128U);
  EXPECT_TRUE(
      CollectValue(Type::kContinuousVectorFileConstantNameAndMemPtr, VectorBytes<kernel::FileConstantNameAndMem>({}))
          .file_constant_name_and_mems.empty());
  const kernel::FileConstantNameAndMem empty_file{"", nullptr, 0U};
  ExpectInvalid(Type::kContinuousVectorFileConstantNameAndMemPtr,
                VectorBytes<kernel::FileConstantNameAndMem>({empty_file}));
}
CustomConstRestoreDescriptor CollectCustom(const Type type, const Bytes &bytes, const char *name) {
  ConstantGraph fixture;
  fixture.Prepare(type, bytes, name);
  EXPECT_EQ(fixture.Collect(), ge::GRAPH_SUCCESS);
  EXPECT_TRUE(fixture.info.const_chains.empty());
  EXPECT_EQ(fixture.info.custom_const_chains.size(), 1U);
  if (fixture.info.custom_const_chains.empty()) {
    return {kInvalidProgramIndex, "", "", true, {0U}};
  }
  const auto &descriptor = fixture.info.custom_const_chains[0];
  EXPECT_EQ(descriptor.chain_index, fixture.Index());
  EXPECT_EQ(descriptor.producer_name, name);
  return descriptor;
}

template <typename T>
T ReadPayload(const Bytes &bytes, const size_t offset) {
  T value{};
  if (offset > bytes.size() || sizeof(value) > bytes.size() - offset) {
    ADD_FAILURE() << "Payload is too short";
    return value;
  }
  EXPECT_EQ(memcpy_s(&value, sizeof(value), bytes.data() + offset, sizeof(value)), EOK);
  return value;
}

TEST(Om2ConstValueCoverage, StorageShapeRestorationCopiesDimensionsAndRejectsOversizedRank) {
  const StorageShape shape{{2, 3}, {6}};
  const auto value = CollectCustom(Type::kGertStorageShapePtr, ObjectBytes(shape), "const_storage_shape_probe");
  EXPECT_EQ(value.initializer, "storage_shape");
  EXPECT_FALSE(value.payload_is_string);
  EXPECT_EQ(value.payload.size(), 2U * sizeof(uint32_t) + 2U * sizeof(int64_t) * Shape::kMaxDimNum);
  EXPECT_EQ(ReadPayload<uint32_t>(value.payload, 0U), 2U);
  EXPECT_EQ(ReadPayload<int64_t>(value.payload, sizeof(uint32_t)), 2);
  EXPECT_EQ(ReadPayload<int64_t>(value.payload, sizeof(uint32_t) + sizeof(int64_t)), 3);
  const auto offset = sizeof(uint32_t) + Shape::kMaxDimNum * sizeof(int64_t);
  EXPECT_EQ(ReadPayload<uint32_t>(value.payload, offset), 1U);
  EXPECT_EQ(ReadPayload<int64_t>(value.payload, offset + sizeof(uint32_t)), 6);
  ExpectInvalid(Type::kGertStorageShapePtr, Replace(ObjectBytes(shape), 0U, size_t{Shape::kMaxDimNum + 1U}),
                "const_storage_shape_probe");
}

TEST(Om2ConstValueCoverage, TensorDataRestorationKeepsBorrowedAddressAndNormalizesEmptyPlacement) {
  int64_t data[] = {31, -41};
  const TensorData source(data, nullptr, sizeof(data), kOnHost);
  const auto value = CollectCustom(Type::kGertTensorDataObjectPtr, ObjectBytes(source), "data");
  EXPECT_EQ(value.initializer, "gert_tensor_data_object");
  EXPECT_TRUE(value.payload_is_string);
  EXPECT_EQ(std::string(value.payload.begin(), value.payload.end() - 1),
            std::to_string(reinterpret_cast<uintptr_t>(data)) + ",16," + std::to_string(kOnHost));
  EXPECT_EQ(value.payload.back(), 0U);
  for (const auto placement : {kOnHost, kTensorPlacementEnd}) {
    const TensorData empty(nullptr, nullptr, 0U, placement);
    const auto restored = CollectCustom(Type::kVoidPtr, ObjectBytes(empty), "empty_data");
    EXPECT_EQ(std::string(restored.payload.begin(), restored.payload.end() - 1),
              "0,0," + std::to_string(kTensorPlacementEnd));
  }
  const size_t placement_offset = 2U * sizeof(void *) + sizeof(size_t);
  ExpectInvalid(Type::kGertTensorDataObjectPtr, Replace(ObjectBytes(source), placement_offset, kTensorPlacementEnd));
  ExpectInvalid(Type::kGertTensorDataObjectPtr, Replace(ObjectBytes(source), placement_offset, int32_t{-1}));
}

TEST(Om2ConstValueCoverage, GertTensorDataRestorationKeepsStreamAndRejectsInvalidPlacement) {
  int64_t data = 17;
  const TensorData source(&data, nullptr, sizeof(data), kOnHost);
  auto bytes = ObjectBytes(source);
  bytes.resize(sizeof(GertTensorData), 0U);
  bytes = Replace(bytes, sizeof(TensorData) + sizeof(uintptr_t), int64_t{13});
  for (const auto *name : {"const_gtd_probe", "const_shape_gtd_host_probe"}) {
    const auto value = CollectCustom(Type::kGertTensorDataPtr, bytes, name);
    EXPECT_EQ(value.initializer, "gert_tensor_data");
    EXPECT_TRUE(value.payload_is_string);
    EXPECT_EQ(std::string(value.payload.begin(), value.payload.end() - 1),
              std::to_string(reinterpret_cast<uintptr_t>(&data)) + ",8," + std::to_string(kOnHost) + ",13");
  }
  const auto invalid = Replace(bytes, 2U * sizeof(void *) + sizeof(size_t), kTensorPlacementEnd);
  ExpectInvalid(Type::kGertTensorDataPtr, invalid, "const_gtd_probe");
}

struct TensorSnapshot {
  uint64_t address;
  uint64_t size;
  int32_t placement;
  int32_t data_type;
  int32_t origin_format;
  int32_t storage_format;
  uint64_t expand_dims_size;
  uint64_t expand_dims_mask;
  uint32_t origin_dim_num;
  uint32_t storage_dim_num;
  int64_t origin_dims[Shape::kMaxDimNum];
  int64_t storage_dims[Shape::kMaxDimNum];
  uint64_t weight_index;
  uint64_t has_weight_index;
  uint64_t weight_data_offset;
  uint64_t weight_data_size;
};

TEST(Om2ConstValueCoverage, TensorRestorationNormalizesShapeOnlyTensorAcrossPolymorphicConsumers) {
  Tensor tensor({{3, 2}, {6}}, {ge::FORMAT_ND, ge::FORMAT_NCHW, {}}, kOnHost, ge::DT_INT32, nullptr);
  tensor.SetSize(0U);
  for (const auto type : {Type::kGertTensorPtr, Type::kGertShapePtr, Type::kVoidPtr}) {
    const auto value = CollectCustom(type, ObjectBytes(tensor), "tensor");
    EXPECT_EQ(value.initializer, "gert_tensor");
    EXPECT_FALSE(value.payload_is_string);
    ASSERT_EQ(value.payload.size(), sizeof(TensorSnapshot));
    const auto snapshot = ReadPayload<TensorSnapshot>(value.payload, 0U);
    EXPECT_EQ(snapshot.address, 0U);
    EXPECT_EQ(snapshot.size, 0U);
    EXPECT_EQ(snapshot.placement, kTensorPlacementEnd);
    EXPECT_EQ(snapshot.data_type, ge::DT_INT32);
    EXPECT_EQ(snapshot.origin_format, ge::FORMAT_ND);
    EXPECT_EQ(snapshot.storage_format, ge::FORMAT_NCHW);
    EXPECT_EQ(snapshot.origin_dim_num, 2U);
    EXPECT_EQ(snapshot.storage_dim_num, 1U);
    EXPECT_EQ(snapshot.origin_dims[0], 3);
    EXPECT_EQ(snapshot.origin_dims[1], 2);
    EXPECT_EQ(snapshot.storage_dims[0], 6);
    EXPECT_EQ(snapshot.has_weight_index, 0U);
  }
  ExpectInvalid(Type::kGertTensorPtr, Replace(ObjectBytes(tensor), 0U, size_t{Shape::kMaxDimNum + 1U}));
}

TEST(Om2ConstValueCoverage, TensorWeightMetadataAndMalformedObjectHeadersAreHandled) {
  int32_t data[] = {2, 3};
  const Tensor tensor({{2}, {2}}, {ge::FORMAT_ND, ge::FORMAT_ND, {}}, kOnHost, ge::DT_INT32, data);
  ConstantGraph fixture;
  ASSERT_NO_FATAL_FAILURE(fixture.Prepare(Type::kGertTensorPtr, ObjectBytes(tensor)));
  auto desc = fixture.constant->GetOpDescBarePtr()->MutableOutputDesc(0U);
  ASSERT_NE(desc, nullptr);
  ASSERT_TRUE(ge::AttrUtils::SetInt(desc, "om2_weight_index", 3));
  ge::TensorUtils::SetDataOffset(*desc, 64);
  ASSERT_EQ(fixture.Collect(), ge::GRAPH_SUCCESS);
  ASSERT_EQ(fixture.info.custom_const_chains.size(), 1U);
  const auto snapshot = ReadPayload<TensorSnapshot>(fixture.info.custom_const_chains[0].payload, 0U);
  EXPECT_EQ(snapshot.address, reinterpret_cast<uintptr_t>(data));
  EXPECT_EQ(snapshot.size, sizeof(data));
  EXPECT_EQ(snapshot.placement, kOnHost);
  EXPECT_EQ(snapshot.has_weight_index, 1U);
  EXPECT_EQ(snapshot.weight_index, 3U);
  EXPECT_EQ(snapshot.weight_data_offset, 64U);
  EXPECT_EQ(snapshot.weight_data_size, sizeof(data));
  const auto version_offset = sizeof(StorageShape) + sizeof(StorageFormat);
  const auto data_offset = version_offset + sizeof(uint32_t) + sizeof(ge::DataType);
  ExpectInvalid(Type::kGertTensorPtr, Replace(ObjectBytes(tensor), version_offset, uint8_t{1U}));
  ExpectInvalid(Type::kGertTensorPtr, Replace(ObjectBytes(tensor), data_offset + sizeof(void *), uintptr_t{1U}));
  ExpectInvalid(Type::kGertTensorPtr,
                Replace(ObjectBytes(tensor), data_offset + 2U * sizeof(void *) + sizeof(size_t), kTensorPlacementEnd));
}

TEST(Om2ConstValueCoverage, ProcessLocalPointersHaveExplicitRestorationAndOpaquePointersMustBeNull) {
  const std::vector<Type> process_types{Type::kVoidPtr,
                                        Type::kKernelFuncPtr,
                                        Type::kGertOpImplSpaceRegistryV2Ptr,
                                        Type::kGertOpImplSpaceRegistryV2ArrayPtr,
                                        Type::kGeDavinciModelPtr,
                                        Type::kGertCustomOpRegistryPtr,
                                        Type::kFePlatFormInfosPtr,
                                        Type::kGertCoreNumInfosPtr,
                                        Type::kGertTensorDataVectorPtr,
                                        Type::kGertRtKernelLaunchArgsExPtr,
                                        Type::kGertTilingFwkDataPtr,
                                        Type::kGertCacheableTilingFwkDataPtr,
                                        Type::kGeOpDescPtr,
                                        Type::kAicpuArgsHandlerPtr,
                                        Type::kGertAicpuExtInfoHandlerPtr,
                                        Type::kRtSessionPtr};
  for (const auto type : process_types) {
    SCOPED_TRACE(static_cast<int>(type));
    const auto null_value = CollectCustom(type, ObjectBytes(uintptr_t{0U}), "pointer");
    EXPECT_EQ(null_value.initializer, "null_pointer");
    EXPECT_EQ(null_value.payload, (Bytes{0U}));
    const auto address = CollectCustom(type, ObjectBytes(uintptr_t{4660U}), "pointer");
    EXPECT_EQ(address.initializer, "process_local_pointer");
    EXPECT_EQ(address.payload, (Bytes{'4', '6', '6', '0', 0U}));
    ExpectInvalid(type, {0U});
  }
  for (const auto type :
       {Type::kGertAllocatorPtr, Type::kGeAllocatorPtr, Type::kGertAllocatorsPtr, Type::kRtStreamTPtr,
        Type::kRtNotifyTPtr, Type::kGertModelDescPtr, Type::kOuterWeightMemPtr, Type::kGertKernelBinDataPtr}) {
    EXPECT_TRUE(CollectValue(type, ObjectBytes(uintptr_t{0U})).is_null_pointer);
    ExpectInvalid(type, ObjectBytes(uintptr_t{4660U}));
  }
}

TEST(Om2ConstValueCoverage, UnsupportedPointerContainersRejectSerializedAddresses) {
  for (const auto type : {Type::kTypedContinuousVectorRtStreamTPtr, Type::kTypedContinuousVectorRtNotifyTPtr,
                          Type::kGertAllocatorVectorPtr, Type::kAicpuHostProcFuncPtr, Type::kGeKernelBinPtr}) {
    ExpectInvalid(type, ObjectBytes(uintptr_t{0U}));
  }
}

void ExpectAllTypedContractConflicts() {
  GertRuntimeStub stub;
  stub.GetSlogStub().SetLevel(DLOG_ERROR);
  stub.GetSlogStub().NoConsoleOut();
  ConstantGraph fixture;
  ASSERT_NO_FATAL_FAILURE(fixture.Prepare(Type::kInt64T, ObjectBytes(int64_t{19})));
  fixture.info.init.kernels = fixture.info.main.kernels;
  // Om2ValueType is contiguous; every declared ABI type must reject an incompatible consumer.
  for (uint32_t index = 0U; index <= static_cast<uint32_t>(Type::kAclnnOriginalDeterministicConfigPtr); ++index) {
    const auto type = static_cast<Type>(index);
    if (type == Type::kUnknown) {
      continue;
    }
    SCOPED_TRACE(index);
    stub.GetSlogStub().Clear();
    fixture.info.init.kernels[0].inputs[0].value_type = type;
    fixture.info.main.kernels[0].inputs[0].value_type = type == Type::kInt64T ? Type::kBool : Type::kInt64T;
    EXPECT_NE(fixture.Collect(), ge::GRAPH_SUCCESS);
    EXPECT_EQ(stub.GetSlogStub().CountLog(DLOG_ERROR, "has conflicting input contracts"), 1);
    EXPECT_TRUE(fixture.info.const_chains.empty());
    EXPECT_TRUE(fixture.info.custom_const_chains.empty());
  }
}

TEST(Om2ConstValueCoverage, ConsumerContractsRejectConflictsAndInvalidIndicesAcrossStages) {
  ASSERT_NO_FATAL_FAILURE(ExpectAllTypedContractConflicts());
  ConstantGraph fixture;
  ASSERT_NO_FATAL_FAILURE(fixture.Prepare(Type::kInt64T, ObjectBytes(int64_t{19})));
  fixture.info.init.kernels = fixture.info.main.kernels;
  fixture.info.deinit.kernels = fixture.info.main.kernels;
  ASSERT_EQ(fixture.Collect(), ge::GRAPH_SUCCESS);
  ASSERT_EQ(fixture.info.const_chains.size(), 1U);
  EXPECT_EQ(fixture.info.const_chains[0].value.signed_value, 19);
  fixture.info.deinit.kernels[0].inputs[0].value_type = Type::kUInt64T;
  EXPECT_NE(fixture.Collect(), ge::GRAPH_SUCCESS);
  fixture.info.chains[fixture.Index()].binding_phase = ChainBindingPhase::kLoad;
  EXPECT_NE(fixture.Collect(), ge::GRAPH_SUCCESS);
  fixture.info.deinit.kernels[0].inputs[0].value_type = Type::kInt64T;
  ASSERT_EQ(fixture.Collect(), ge::GRAPH_SUCCESS);
  EXPECT_TRUE(fixture.info.const_chains.empty());
  fixture.info.main.kernels[0].inputs[0].chain_index = kInvalidProgramIndex;
  EXPECT_NE(fixture.Collect(), ge::GRAPH_SUCCESS);
}

TEST(Om2ConstValueCoverage, TensorAndVoidConsumersReconcileRegardlessOfVisitOrder) {
  const Tensor tensor({{2}, {2}}, {ge::FORMAT_ND, ge::FORMAT_ND, {}}, ge::DT_INT32);
  for (const auto first : {Type::kVoidPtr, Type::kGertTensorPtr}) {
    ConstantGraph fixture;
    ASSERT_NO_FATAL_FAILURE(fixture.Prepare(first, ObjectBytes(tensor)));
    fixture.info.init.kernels = fixture.info.main.kernels;
    const auto second = first == Type::kVoidPtr ? Type::kGertTensorPtr : Type::kVoidPtr;
    fixture.info.main.kernels[0].inputs[0].value_type = second;
    ASSERT_EQ(fixture.Collect(), ge::GRAPH_SUCCESS);
    ASSERT_EQ(fixture.info.custom_const_chains.size(), 1U);
    EXPECT_EQ(fixture.info.custom_const_chains[0].initializer, "gert_tensor");
    EXPECT_TRUE(fixture.info.const_chains.empty());
  }
}

TEST(Om2ConstValueCoverage, CustomRestorationDeduplicatesAndRejectsDescriptorDisagreement) {
  ConstantGraph fixture;
  ASSERT_NO_FATAL_FAILURE(fixture.Prepare(Type::kVoidPtr, ObjectBytes(uintptr_t{13U})));
  CustomConstRestoreDescriptor descriptor{fixture.Index(), "value", "probe", true, {'x', 0U}};
  fixture.info.main.kernels[0].custom_const_restore_descriptors = {descriptor};
  fixture.info.init.kernels = fixture.info.main.kernels;
  fixture.info.deinit.kernels = fixture.info.main.kernels;
  ASSERT_EQ(fixture.Collect(), ge::GRAPH_SUCCESS);
  ASSERT_EQ(fixture.info.custom_const_chains.size(), 1U);
  EXPECT_EQ(fixture.info.custom_const_chains[0].initializer, "probe");
  EXPECT_EQ(fixture.info.custom_const_chains[0].payload, descriptor.payload);
  auto changed = descriptor;
  changed.payload = {'y', 0U};
  fixture.info.deinit.kernels[0].custom_const_restore_descriptors = {changed};
  EXPECT_NE(fixture.Collect(), ge::GRAPH_SUCCESS);
  fixture.info.init.kernels.clear();
  fixture.info.deinit.kernels.clear();
  descriptor.producer_name = "wrong_producer";
  fixture.info.main.kernels[0].custom_const_restore_descriptors = {descriptor};
  EXPECT_NE(fixture.Collect(), ge::GRAPH_SUCCESS);
}

TEST(Om2ConstValueCoverage, CompileTimeAndUntypedConstantsAreSkippedOnlyForDeclaredConventions) {
  ConstantGraph fixture;
  ASSERT_NO_FATAL_FAILURE(fixture.Prepare(Type::kUnknown, {13U}));
  EXPECT_NE(fixture.Collect(), ge::GRAPH_SUCCESS);
  fixture.info.main.kernels[0].compile_time_const_chain_indices = {fixture.Index()};
  ASSERT_EQ(fixture.Collect(), ge::GRAPH_SUCCESS);
  EXPECT_TRUE(fixture.info.const_chains.empty());
  for (const auto *name : {"const_build_tensor_attr", "const_dtype_float", "const_unused", "Const_unused"}) {
    ConstantGraph unused;
    ASSERT_NO_FATAL_FAILURE(unused.Prepare(Type::kUnknown, {13U}, name));
    ASSERT_EQ(unused.Collect(), ge::GRAPH_SUCCESS);
    EXPECT_TRUE(unused.info.const_chains.empty());
  }
  ConstantGraph opdef;
  ASSERT_NO_FATAL_FAILURE(opdef.Prepare(Type::kVoidPtr, {3U, 0U, 255U}, "const_op_def_test"));
  ASSERT_EQ(opdef.Collect(), ge::GRAPH_SUCCESS);
  ASSERT_EQ(opdef.info.const_chains.size(), 1U);
  EXPECT_EQ(opdef.info.const_chains[0].value.type, Type::kBytePtr);
  EXPECT_EQ(opdef.info.const_chains[0].value.raw_bytes, (Bytes{3U, 0U, 255U}));
}

TEST(Om2ConstValueCoverage, IfSelectorsAreTypedConsumersAndMissingValueAttributesFail) {
  ConstantGraph fixture;
  ASSERT_NO_FATAL_FAILURE(fixture.Prepare(Type::kUInt32T, ObjectBytes(uint32_t{1U})));
  ProgramStep selector;
  selector.kind = ProgramStepKind::kIf;
  selector.if_info.selector_chain_index = fixture.Index();
  selector.if_info.selector_value_type = Type::kUInt32T;
  fixture.info.main.regions.resize(1U);
  fixture.info.main.regions[0].steps = {selector};
  fixture.info.main.kernels.clear();
  ASSERT_EQ(fixture.Collect(), ge::GRAPH_SUCCESS);
  ASSERT_EQ(fixture.info.const_chains.size(), 1U);
  EXPECT_EQ(fixture.info.const_chains[0].value.unsigned_value, 1U);
  ASSERT_EQ(fixture.constant->GetOpDescBarePtr()->DelAttr(kConstValue), ge::GRAPH_SUCCESS);
  EXPECT_NE(fixture.Collect(), ge::GRAPH_SUCCESS);
}
}  // namespace
}  // namespace om2
}  // namespace gert
