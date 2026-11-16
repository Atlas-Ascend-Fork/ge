/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#include "program_body_codegen_fixture.h"
#include "securec.h"

namespace gert {
namespace om2 {
namespace {
ProgramBodyCodegenOption CoverageOptions() {
  ProgramBodyCodegenOption option;
  option.model_name = "constant_coverage";
  option.interface_file_name = "constant_coverage.h";
  option.source_file_name = "constant_coverage.cpp";
  option.makefile_file_name = "Makefile";
  option.emit_makefile = false;
  return option;
}

std::string GeneratedText(const ProgramBodyCodegenResult &result) {
  std::string text;
  for (const auto &artifact : result.artifacts) {
    text.append(reinterpret_cast<const char *>(artifact.data.get()), artifact.data_size);
  }
  return text;
}

ProgramInfo ConstantProgram() {
  auto program = program_body_fixture::RegistryBindingProgram(false);
  program.main = {};
  program.main.regions.resize(1U);
  program.main.root_region_index = 0U;
  program.chains.resize(1U);
  program.chains[0].source_kind = ChainSourceKind::kConstant;
  program.chains[0].binding_kind = ChainBindingKind::kNone;
  program.chains[0].binding_phase = ChainBindingPhase::kNone;
  return program;
}

void CheckConstant(const Om2ConstValue &value, const std::string &expected) {
  auto program = ConstantProgram();
  program.const_chains.push_back({0U, "constant", value});
  ProgramBodyCodegenResult result;
  ASSERT_EQ(ProgramBodyCodegen().Generate(program, CoverageOptions(), result), ge::SUCCESS);
  EXPECT_NE(GeneratedText(result).find(expected), std::string::npos) << expected;
}

TEST(ProgramCodegenCoverage, EmitsScalarConstantsWithTheirTypedValues) {
  const std::vector<std::pair<Om2ValueType, std::string>> cases = {
      {Om2ValueType::kBool, "&InitInlineConst<bool>, (const bool[]){true}"},
      {Om2ValueType::kInt32T, "&InitInlineConst<int32_t>, (const int32_t[]){-19}"},
      {Om2ValueType::kInt64T, "&InitInlineConst<int64_t>, (const int64_t[]){-19}"},
      {Om2ValueType::kUInt32T, "&InitInlineConst<uint32_t>, (const uint32_t[]){23U}"},
      {Om2ValueType::kUInt64T, "&InitInlineConst<uint64_t>, (const uint64_t[]){23U}"},
      {Om2ValueType::kSizeT, "&InitInlineConst<size_t>, (const size_t[]){23U}"},
      {Om2ValueType::kGeDataType, "&InitInlineConst<uint32_t>, (const uint32_t[]){23U}"},
      {Om2ValueType::kGertTensorPlacement, "&InitInlineConst<uint32_t>, (const uint32_t[]){23U}"},
      {Om2ValueType::kGertCoreTypeIndexPtr, "&InitInlineConst<uint32_t>, (const uint32_t[]){23U}"},
      {Om2ValueType::kGeOppImplVersion, "&InitInlineConst<uint32_t>, (const uint32_t[]){23U}"}};
  for (const auto &item : cases) {
    SCOPED_TRACE(static_cast<int>(item.first));
    Om2ConstValue value;
    value.type = item.first;
    value.signed_value = -19;
    value.unsigned_value = 23U;
    CheckConstant(value, item.second);
  }
}

TEST(ProgramCodegenCoverage, EmitsStructuredConstantInitializers) {
  const std::vector<std::pair<Om2ValueType, std::string>> cases = {
      {Om2ValueType::kRtTaskCfgInfoTPtr, "&InitTaskConfigConst, nullptr"},
      {Om2ValueType::kAclrtLaunchKernelCfgPtr, "&InitLaunchKernelConfigConst, (const size_t[]){0U}"},
      {Om2ValueType::kAclrtLaunchKernelAttrPtr, "&InitLaunchKernelAttrsConst,"},
      {Om2ValueType::kGertDfxExeArgPtr, "&InitDfxExeArgConst,"},
      {Om2ValueType::kGertBuildTensorAttrPtr, "&InitBuildTensorAttrConst,"},
      {Om2ValueType::kRtKernelLaunchArgsExComputeNodeDescPtr, "&InitComputeNodeDescConst,"},
      {Om2ValueType::kGertArgsInfosDescPtr, "&InitArgsInfosDescConst,"},
      {Om2ValueType::kGertContinuousVectorVectorInt32Ptr, "&InitContinuousVectorVectorInt32Const,"},
      {Om2ValueType::kTypedContinuousVectorInt64Ptr, "&InitContinuousVectorConst<int64_t>,"},
      {Om2ValueType::kTypedContinuousVectorSizeTPtr, "&InitContinuousVectorConst<size_t>,"},
      {Om2ValueType::kTypedContinuousVectorArgsIndexToIoIndexPtr,
       "&InitContinuousVectorConst<GeneratedArgsIndexToIoIndex>,"},
      {Om2ValueType::kGertOpExecuteOptionsPtr, "&InitOpExecuteOptionsConst,"},
      {Om2ValueType::kGertMixCoreArgsPtr, "&InitMixCoreArgsConst,"},
      {Om2ValueType::kAclnnDeterministicConfigPtr, "&InitOwnedConst<gert::kernel::AclnnDeterministicConfig>,"},
      {Om2ValueType::kAclnnOriginalDeterministicConfigPtr,
       "&InitOwnedConst<gert::kernel::AclnnOriginalDeterministicConfig>,"},
      {Om2ValueType::kContinuousVectorFileConstantNameAndMemPtr,
       "&InitContinuousVectorConst<GeneratedFileConstantNameAndMem>,"}};
  for (const auto &item : cases) {
    SCOPED_TRACE(static_cast<int>(item.first));
    Om2ConstValue value;
    value.type = item.first;
    CheckConstant(value, item.second);
  }
}

TEST(ProgramCodegenCoverage, EmitsNonemptyVectorAndMemoryDescriptors) {
  Om2ConstValue value;
  value.type = Om2ValueType::kTypedContinuousVectorInt64Ptr;
  value.continuous_vector_int64 = {-3, 7};
  CheckConstant(value, "(const int64_t[]){-3, 7}");
  value.type = Om2ValueType::kTypedContinuousVectorSizeTPtr;
  value.continuous_vector_size_t = {2U, 9U};
  CheckConstant(value, "(const size_t[]){2, 9}");
  value.type = Om2ValueType::kGertContinuousVectorVectorInt32Ptr;
  value.continuous_vector_vector_int32 = {{}, {5, -8}};
  CheckConstant(value, "(const int32_t[]){5, -8}");
  value.type = Om2ValueType::kMemoryBaseTypeOffsetPtr;
  value.memory_base_type_offsets = {{1U, -9, 16U}, {2U, 32, 64U}};
  CheckConstant(value, "{1U, -9LL, 16ULL}, {2U, 32LL, 64ULL}");
  value.type = Om2ValueType::kContinuousVectorFileConstantNameAndMemPtr;
  value.file_constant_name_and_mems = {{"weight", 0U, 16U}, {"other", 0U, 8U}};
  CheckConstant(value, "{\"weight\", (const void *)(uintptr_t)0ULL, 16U}");
  value.type = Om2ValueType::kTypedContinuousVectorArgsIndexToIoIndexPtr;
  value.continuous_vector_args_index_to_io_index = {{1, 2U, 3U}, {4, 5U, 6U}};
  CheckConstant(value, "{1, 2U, 3U}, {4, 5U, 6U}");
}

TEST(ProgramCodegenCoverage, EmitsBytePayloadsAndNestedArgumentMetadata) {
  Om2ConstValue value;
  value.type = Om2ValueType::kBytePtr;
  CheckConstant(value, "&InitRawBytesConst, (const uint8_t[]){0U}");
  value.raw_bytes = {3U, 255U};
  CheckConstant(value, "&InitRawBytesConst, (const uint8_t[]){3U, 255U}");
  value.type = Om2ValueType::kGertOpExecuteOptionsPtr;
  value.op_execute_options = {9U, 2U};
  CheckConstant(value, "(const uint8_t[]){9U, 2U}");
  value.type = Om2ValueType::kRtKernelLaunchArgsExComputeNodeDescPtr;
  value.compute_node_desc.compiled_args = {6U, 7U};
  CheckConstant(value, "(const uint8_t[]){6U, 7U}");
  value.type = Om2ValueType::kGertArgsInfosDescPtr;
  value.args_infos_desc.args_infos.resize(2U);
  value.args_infos_desc.args_infos[0].dyn_desc = {true, 1U, 2U, 3U, 4U, 5U, 6U};
  CheckConstant(value, "{true, 1U, 2U, 3U, 4U, 5U, 6U}");
  value.type = Om2ValueType::kAclrtLaunchKernelAttrPtr;
  value.launch_kernel_attrs = {{1, 7U}, {2, 8U}};
  CheckConstant(value, "&InitLaunchKernelAttrsConst,");
}

TEST(ProgramCodegenCoverage, RejectsNonportableValuesButAcceptsNullPointer) {
  for (const auto type :
       {Om2ValueType::kVoidPtr, Om2ValueType::kGertAllocatorPtr, Om2ValueType::kMemoryBaseTypeOffsetPtr}) {
    auto program = ConstantProgram();
    Om2ConstValue value;
    value.type = type;
    program.const_chains.push_back({0U, "invalid_constant", value});
    ProgramBodyCodegenResult result;
    result.artifacts.resize(1U);
    EXPECT_NE(ProgramBodyCodegen().Generate(program, CoverageOptions(), result), ge::SUCCESS);
    EXPECT_TRUE(result.artifacts.empty());
  }
  Om2ConstValue value;
  value.type = Om2ValueType::kVoidPtr;
  value.is_null_pointer = true;
  CheckConstant(value, "&InitNullConst, nullptr");
}

CustomConstRestoreDescriptor StringRestore(const std::string &initializer, const std::string &payload) {
  CustomConstRestoreDescriptor descriptor;
  descriptor.chain_index = 0U;
  descriptor.producer_name = "restored_constant";
  descriptor.initializer = initializer;
  descriptor.payload.assign(payload.begin(), payload.end());
  descriptor.payload.push_back(0U);
  return descriptor;
}

TEST(ProgramCodegenCoverage, EmitsCustomRestoreFunctionAndParsedTensorData) {
  const std::vector<std::pair<std::string, std::string>> cases = {
      {"aicpu_host_proc_func", "&InitAicpuHostProcFuncConst,"},
      {"tensor_data_offset", "&InitTensorDataOffsetConst,"},
      {"process_local_pointer", "&InitProcessLocalPointerConst,"},
      {"null_pointer", "&InitNullPointerConst,"},
      {"CustomRestore", "&CustomRestore,"},
      {"gert_tensor_data", "(const GertTensorDataConstDef[]){ {0ULL, 0ULL, 0, -1LL} }"},
      {"gert_tensor_data_object", "(const TensorDataObjectConstDef[]){ {0ULL, 0ULL, 0} }"}};
  for (const auto &item : cases) {
    auto program = ConstantProgram();
    const auto payload = item.first == "gert_tensor_data_object" ? "0,0,0" : "0,0,0,-1";
    program.custom_const_chains.push_back(StringRestore(item.first, payload));
    ProgramBodyCodegenResult result;
    ASSERT_EQ(ProgramBodyCodegen().Generate(program, CoverageOptions(), result), ge::SUCCESS);
    EXPECT_NE(GeneratedText(result).find(item.second), std::string::npos) << item.first;
  }
}

TEST(ProgramCodegenCoverage, RejectsMalformedCustomRestorePayloadsWithoutArtifacts) {
  std::vector<CustomConstRestoreDescriptor> cases = {StringRestore("", "value"),
                                                     StringRestore("storage_shape", "short"),
                                                     StringRestore("gert_tensor", "short"),
                                                     StringRestore("gert_tensor_data", "not,a,tensor"),
                                                     StringRestore("gert_tensor_data", "0,0,-1,0"),
                                                     StringRestore("gert_tensor_data_object", "0,0,0,trailing"),
                                                     StringRestore("gert_tensor_data_object", "0,1,-1")};
  auto missing_terminator = StringRestore("null_pointer", "0");
  missing_terminator.payload.pop_back();
  cases.push_back(missing_terminator);
  missing_terminator.payload.clear();
  cases.push_back(missing_terminator);
  for (const auto &descriptor : cases) {
    SCOPED_TRACE(descriptor.initializer);
    auto program = ConstantProgram();
    program.custom_const_chains = {descriptor};
    ProgramBodyCodegenResult result;
    result.artifacts.resize(1U);
    EXPECT_NE(ProgramBodyCodegen().Generate(program, CoverageOptions(), result), ge::SUCCESS);
    EXPECT_TRUE(result.artifacts.empty());
  }
}

TEST(ProgramCodegenCoverage, EmitsTensorShapesAndRejectsOversizedRank) {
  Om2ConstValue value;
  value.type = Om2ValueType::kGertTensorPtr;
  value.tensor.origin_shape = {2, 3};
  value.tensor.storage_shape = {6};
  value.tensor.placement = -1;
  value.tensor.has_weight_index = true;
  value.tensor.weight_index = 3U;
  value.tensor.data_offset = 8U;
  value.tensor.data_size = 24U;
  CheckConstant(value, "2U, 1U, {2LL, 3LL, 0LL");
  auto program = ConstantProgram();
  value.tensor.origin_shape.resize(gert::Shape::kMaxDimNum + 1U);
  program.const_chains.push_back({0U, "oversized_tensor", value});
  ProgramBodyCodegenResult result;
  EXPECT_NE(ProgramBodyCodegen().Generate(program, CoverageOptions(), result), ge::SUCCESS);
  EXPECT_TRUE(result.artifacts.empty());
}

TEST(ProgramCodegenCoverage, RestoresBinaryStorageShapeAndValidatesDimensionCount) {
  auto descriptor = StringRestore("storage_shape", "");
  descriptor.payload_is_string = false;
  descriptor.payload.assign(2U * sizeof(uint32_t) + 2U * gert::Shape::kMaxDimNum * sizeof(int64_t), 0U);
  uint32_t rank = 1U;
  int64_t dim = 37;
  ASSERT_EQ(memcpy_s(descriptor.payload.data(), descriptor.payload.size(), &rank, sizeof(rank)), EOK);
  ASSERT_EQ(
      memcpy_s(descriptor.payload.data() + sizeof(rank), descriptor.payload.size() - sizeof(rank), &dim, sizeof(dim)),
      EOK);
  auto program = ConstantProgram();
  program.custom_const_chains = {descriptor};
  ProgramBodyCodegenResult result;
  ASSERT_EQ(ProgramBodyCodegen().Generate(program, CoverageOptions(), result), ge::SUCCESS);
  EXPECT_NE(GeneratedText(result).find("&InitStorageShapeConst, (const StorageShapeConstDef[]){{1U, 0U, {37"),
            std::string::npos);
  rank = gert::Shape::kMaxDimNum + 1U;
  ASSERT_EQ(memcpy_s(program.custom_const_chains[0].payload.data(), program.custom_const_chains[0].payload.size(),
                     &rank, sizeof(rank)),
            EOK);
  EXPECT_NE(ProgramBodyCodegen().Generate(program, CoverageOptions(), result), ge::SUCCESS);
  EXPECT_TRUE(result.artifacts.empty());
}

TEST(ProgramCodegenCoverage, EmitsBinaryCustomPayloadAndEscapedString) {
  auto program = ConstantProgram();
  auto descriptor = StringRestore("CustomBytes", "");
  descriptor.payload_is_string = false;
  descriptor.payload = {2U, 4U, 255U};
  program.custom_const_chains = {descriptor};
  ProgramBodyCodegenResult result;
  ASSERT_EQ(ProgramBodyCodegen().Generate(program, CoverageOptions(), result), ge::SUCCESS);
  EXPECT_NE(GeneratedText(result).find("&CustomBytes, (const uint8_t[]){2U, 4U, 255U}"), std::string::npos);
  Om2ConstValue value;
  value.type = Om2ValueType::kCStringPtr;
  value.string_value = "quote\" slash\\ line\n";
  CheckConstant(value, "quote\\\" slash\\\\ line\\n");
}

class FusionCoverageBuilder final : public KernelCodeBuilder {
 public:
  explicit FusionCoverageBuilder(std::string body) : body_(std::move(body)) {}
  ge::graphStatus Collect(const KernelCollectContext &, KernelProgramInfo &) override {
    return ge::GRAPH_PARAM_INVALID;
  }
  ge::graphStatus GenerateKernelCode(const std::string &symbol, std::string &code) const override {
    code = "  aclError " + symbol + "() {\n" + body_ + "  }\n";
    return ge::GRAPH_SUCCESS;
  }
  ge::graphStatus GenerateOutputsCreatorCode(std::string &code) const override {
    code = "CreateCoverageOutput()";
    return ge::GRAPH_SUCCESS;
  }

 private:
  std::string body_;
};

ProgramInfo FusionProgram(const std::string &producer_return = "kOk") {
  auto program = ConstantProgram();
  program.chains[0].source_kind = ChainSourceKind::kKernelOutput;
  KernelProgramInfo producer;
  producer.execution_node_name = "producer";
  producer.kernel_type = "FusionProducer";
  producer.impl_type = KernelImplType::kCodegenImpl;
  producer.output_policy = OutputCreatePolicy::kTemporary;
  producer.outputs = {{0U, "intermediate", Om2ValueType::kInt32T}};
  producer.code_builder = std::make_shared<FusionCoverageBuilder>(
      "    *chains_[0U].GetPointer<int32_t>() = 29;\n    return " + producer_return + ";\n");
  auto consumer = producer;
  consumer.execution_node_name = "consumer";
  consumer.kernel_type = "FusionConsumer";
  consumer.inputs = producer.outputs;
  consumer.outputs.clear();
  consumer.code_builder = std::make_shared<FusionCoverageBuilder>(
      "    if (*chains_[0U].GetPointer<int32_t>() != 29) { return kFailed; }\n    return kOk;\n");
  program.main.kernels = {producer, consumer};
  ProgramStep first;
  first.kernel_index = 0U;
  ProgramStep second;
  second.kernel_index = 1U;
  program.main.regions = {{0U, "fusion", {first, second}}};
  program.main.root_region_index = 0U;
  program.main.kernel_dependencies = {{0U, 1U}};
  return program;
}

TEST(ProgramCodegenCoverage, FusesProducerConsumerWithLocalIntermediateAndFallthrough) {
  ProgramBodyCodegenResult result;
  ASSERT_EQ(ProgramBodyCodegen().Generate(FusionProgram(), CoverageOptions(), result), ge::SUCCESS);
  const auto text = GeneratedText(result);
  EXPECT_NE(text.find("gert::Chain fused_chain_0{};"), std::string::npos);
  EXPECT_NE(text.find("*fused_chain_0.GetPointer<int32_t>() = 29;"), std::string::npos);
  EXPECT_NE(text.find("if (*fused_chain_0.GetPointer<int32_t>() != 29) { return kFailed; }"), std::string::npos);
  EXPECT_NE(text.find("break;"), std::string::npos);
  EXPECT_NE(text.find("return RunKernelImplF0();"), std::string::npos);
}

TEST(ProgramCodegenCoverage, PreservesUnfusedCodeWhenReturnCannotBeRewritten) {
  ProgramBodyCodegenResult result;
  ASSERT_EQ(ProgramBodyCodegen().Generate(FusionProgram("ACL_SUCCESS"), CoverageOptions(), result), ge::SUCCESS);
  const auto text = GeneratedText(result);
  EXPECT_EQ(text.find("gert::Chain fused_chain_0{};"), std::string::npos);
  EXPECT_NE(text.find("*chains_[0U].GetPointer<int32_t>() = 29;"), std::string::npos);
  EXPECT_NE(text.find("return ACL_SUCCESS;"), std::string::npos);
}

TEST(ProgramCodegenCoverage, KeepsSharedIntermediateGlobal) {
  auto program = FusionProgram();
  program.main.kernels[1].inputs.push_back(program.main.kernels[1].inputs.front());
  ProgramBodyCodegenResult result;
  ASSERT_EQ(ProgramBodyCodegen().Generate(program, CoverageOptions(), result), ge::SUCCESS);
  const auto text = GeneratedText(result);
  EXPECT_EQ(text.find("gert::Chain fused_chain_0{};"), std::string::npos);
  EXPECT_NE(text.find("*chains_[0U].GetPointer<int32_t>() = 29;"), std::string::npos);
}

TEST(ProgramCodegenCoverage, EmitsMultithreadPlanForDependentOwnKernels) {
  auto program = FusionProgram();
  program.main.plan_type = ProgramPlanType::kStructuredTopological;
  auto option = CoverageOptions();
  option.thread_num = 2U;
  ProgramBodyCodegenResult result;
  ASSERT_EQ(ProgramBodyCodegen().Generate(program, option, result), ge::SUCCESS);
  const auto text = GeneratedText(result);
  EXPECT_NE(text.find("29;"), std::string::npos);
  EXPECT_NE(text.find("FusionProducer"), std::string::npos);
  EXPECT_NE(text.find("FusionConsumer"), std::string::npos);
  EXPECT_NE(text.find("static constexpr size_t kWmtWorkerCount = "), std::string::npos);
  EXPECT_NE(text.find("MtRow{MtOp::kRunKernelImpl,"), std::string::npos);
}

TEST(ProgramCodegenCoverage, EmitsBinaryBindingsAndOrdersRuntimeSlotsBySourceIndex) {
  auto program = ConstantProgram();
  program.chains = {
      {0U, 1U, ChainSourceKind::kRuntimeBinding, ChainBindingPhase::kLoad, ChainBindingKind::kStream, 1},
      {1U, 2U, ChainSourceKind::kRuntimeBinding, ChainBindingPhase::kLoad, ChainBindingKind::kStream, 0},
      {2U, 3U, ChainSourceKind::kRuntimeBinding, ChainBindingPhase::kLoad, ChainBindingKind::kBinary, 0},
      {3U, 4U, ChainSourceKind::kRuntimeBinding, ChainBindingPhase::kLoad, ChainBindingKind::kGeKernelBinary, 0}};
  program.binary_resource_keys = {{2U, "aicore\"binary"}, {3U, "ge_binary"}};
  ProgramBodyCodegenResult result;
  ASSERT_EQ(ProgramBodyCodegen().Generate(program, CoverageOptions(), result), ge::SUCCESS);
  const auto text = GeneratedText(result);
  EXPECT_NE(text.find("(const uint32_t[]){1U, 0U}"), std::string::npos);
  EXPECT_NE(text.find("{2U, \"aicore\\\"binary\", false}"), std::string::npos);
  EXPECT_NE(text.find("{3U, \"ge_binary\", true}"), std::string::npos);
  EXPECT_NE(text.find("#include \"graph/op_kernel_bin.h\""), std::string::npos);
}

TEST(ProgramCodegenCoverage, RejectsMissingOptionsCorruptResourcesAndMismatchedModelIo) {
  const std::vector<std::string ProgramBodyCodegenOption::*> fields = {
      &ProgramBodyCodegenOption::model_name, &ProgramBodyCodegenOption::interface_file_name,
      &ProgramBodyCodegenOption::source_file_name, &ProgramBodyCodegenOption::makefile_file_name};
  for (const auto field : fields) {
    auto option = CoverageOptions();
    (option.*field).clear();
    ProgramBodyCodegenResult result;
    result.artifacts.resize(1U);
    EXPECT_NE(ProgramBodyCodegen().Generate(ConstantProgram(), option, result), ge::SUCCESS);
    EXPECT_TRUE(result.artifacts.empty());
  }
  auto corrupt = ConstantProgram();
  corrupt.resources.buffer = {0U};
  auto mismatch = ConstantProgram();
  mismatch.chains[0].source_kind = ChainSourceKind::kModelOutput;
  mismatch.chains[0].source_index = 0;
  for (const auto &program : {corrupt, mismatch}) {
    ProgramBodyCodegenResult result;
    EXPECT_NE(ProgramBodyCodegen().Generate(program, CoverageOptions(), result), ge::SUCCESS);
    EXPECT_TRUE(result.artifacts.empty());
  }
}

TEST(ProgramCodegenCoverage, PreservesCrossWorkerDependencyWaitsWithoutLocalizingChain) {
  auto option = CoverageOptions();
  option.thread_num = 4U;
  auto program = FusionProgram();
  program.main.plan_type = ProgramPlanType::kStructuredTopological;
  ProgramBodyCodegenResult result;
  ASSERT_EQ(ProgramBodyCodegen().Generate(program, option, result), ge::SUCCESS);
  const auto text = GeneratedText(result);
  EXPECT_NE(text.find("MtRow{MtOp::kWait,"), std::string::npos);
  EXPECT_NE(text.find("MtRow{MtOp::kRunKernelImpl, 0U,"), std::string::npos);
  EXPECT_NE(text.find("MtRow{MtOp::kRunKernelImpl, 1U,"), std::string::npos);
  EXPECT_EQ(text.find("gert::Chain fused_chain_0{};"), std::string::npos);
}

TEST(ProgramCodegenCoverage, EmitsOutputCreationAndRejectsMissingCodeBuilder) {
  auto program = FusionProgram();
  program.main.kernels[0].output_policy = OutputCreatePolicy::kCodegenImpl;
  program.main.kernels[1].output_policy = OutputCreatePolicy::kOutputsCreator;
  ProgramBodyCodegenResult result;
  ASSERT_EQ(ProgramBodyCodegen().Generate(program, CoverageOptions(), result), ge::SUCCESS);
  const auto text = GeneratedText(result);
  EXPECT_NE(text.find("OM2_ASSERT_SUCCESS(CreateCoverageOutput());"), std::string::npos);
  EXPECT_NE(text.find("OM2_ASSERT_SUCCESS(CreateContextOutput(kOm2Model.program.main[1U]));"), std::string::npos);
  EXPECT_EQ(text.find("gert::Chain fused_chain_0{};"), std::string::npos);
  program.main.kernels[0].code_builder.reset();
  EXPECT_NE(ProgramBodyCodegen().Generate(program, CoverageOptions(), result), ge::SUCCESS);
  EXPECT_TRUE(result.artifacts.empty());
}

ProgramInfo IslandProgram() {
  auto program = FusionProgram();
  program.main.plan_type = ProgramPlanType::kStructuredTopological;
  program.chains.push_back({1U, 2U, ChainSourceKind::kConstant});
  Om2ConstValue selector;
  selector.type = Om2ValueType::kUInt32T;
  program.const_chains.push_back({1U, "selector", selector});
  ProgramStep step;
  step.kind = ProgramStepKind::kIf;
  step.if_info.selector_chain_index = 1U;
  step.if_info.selector_value_type = Om2ValueType::kUInt32T;
  IfProgramInfo::Branch first;
  first.selector_value = 0U;
  first.region_index = 1U;
  step.if_info.branches = {first};
  const auto steps = program.main.regions[0].steps;
  program.main.regions = {{0U, "root", {step}}, {1U, "branch", steps}};
  return program;
}

TEST(ProgramCodegenCoverage, GeneratesFusedIfIslandForSerialAndMultithreadExecution) {
  for (const auto threads : {1U, 2U}) {
    auto option = CoverageOptions();
    option.thread_num = threads;
    ProgramBodyCodegenResult result;
    ASSERT_EQ(ProgramBodyCodegen().Generate(IslandProgram(), option, result), ge::SUCCESS);
    const auto text = GeneratedText(result);
    EXPECT_NE(text.find("switch (chains_[1U].GetValue<uint32_t>())"), std::string::npos);
    EXPECT_NE(text.find("gert::Chain fused_chain_0{};"), std::string::npos);
    if (threads > 1U) {
      EXPECT_NE(text.find("MtRow{MtOp::kRunIsland,"), std::string::npos);
      EXPECT_NE(text.find("kernel_ret = RunMainStructuredIsland0();"), std::string::npos);
    }
  }
}

TEST(ProgramCodegenCoverage, RejectsInvalidSelectorAndDuplicateBranchLabels) {
  auto invalid_type = IslandProgram();
  invalid_type.main.regions[0].steps[0].if_info.selector_value_type = Om2ValueType::kInt64T;
  auto duplicate = IslandProgram();
  auto &branches = duplicate.main.regions[0].steps[0].if_info.branches;
  branches.push_back(branches.front());
  auto invalid_kernel = FusionProgram();
  invalid_kernel.main.regions[0].steps.resize(1U);
  invalid_kernel.main.regions[0].steps[0].kernel_index = 99U;
  for (const auto &program : {invalid_type, duplicate, invalid_kernel}) {
    ProgramBodyCodegenResult result;
    EXPECT_NE(ProgramBodyCodegen().Generate(program, CoverageOptions(), result), ge::SUCCESS);
    EXPECT_TRUE(result.artifacts.empty());
  }
}
}  // namespace
}  // namespace om2
}  // namespace gert
