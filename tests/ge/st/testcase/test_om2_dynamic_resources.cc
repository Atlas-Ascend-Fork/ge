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
#include <array>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <set>
#include <string>
#include <vector>

#include "ge/ge_api_v2.h"
#include "ge/ge_ir_build.h"
#include "init_ge.h"
#include "framework/common/gert_model_data_deserialize.h"
#include "framework/common/gert_model_data_serialize.h"
#include "framework/runtime/om2_model_executor.h"
#include "graph/attr_value.h"
#include "graph/tensor.h"
#include "session/ge_session_impl.h"
#include "common/om2/dynamic_codegen/info_collect/kernel/common_collector_utils.h"
#include "common/om2/dynamic_codegen/info_collect/kernel_code_builder.h"
#include "common/om2/dynamic_codegen/prepare/gert_graph_attrs.h"
#include "common/om2/dynamic_codegen/prepare/gert_metadata_preparer.h"
#include "exe_graph/runtime/continuous_vector.h"
#include "framework/common/helper/om2_package_helper.h"
#include "common/helper/om2/om2_utils.h"
#include "framework/runtime/model_desc.h"
#include "framework/runtime/model_v2_executor.h"
#include "framework/runtime/rt_session.h"
#include "ge/eager_style_graph_builder/cpp/es_graph_builder.h"
#include "graph/fast_graph/gert_graph_attributes.h"
#include "graph/utils/execute_graph_utils.h"
#include "graph/utils/graph_utils_ex.h"
#include "register/kernel_registry.h"
#include "stub/gert_runtime_stub.h"
#include "../../common/om2_dynamic_host_graph.h"
#include "../../common/om2_generated_model_runner.h"
#include "../../ut/ge/graph/load/om2_package_test_utils.h"

namespace ge {
namespace {
class Om2DynamicResourcesST : public om2_test::DynamicPackageTest {
 protected:
  void SetUp() override {
    DynamicPackageTest::SetUp();
    GetThreadLocalContext().SetGraphOption(
        {{"ge.modelFormat", std::to_string(MODEL_FORMAT_OM2)},
         {"ge.buildConfig", "make -s CXXFLAGS='-std=c++17 -fPIC -O0 -D_GLIBCXX_USE_CXX11_ABI=0'"}});
  }
};

void BuildKnownRoot(GeRootModelPtr &root) {
  auto graph = gert::ShareGraph::BuildWithKnownSubgraph();
  ASSERT_NE(graph, nullptr);
  ASSERT_EQ(graph->GetAllSubgraphs().size(), 1U);
  const auto subgraph = graph->GetAllSubgraphs().front();
  const auto constant = subgraph->FindNode("const1");
  ASSERT_NE(constant, nullptr);
  const std::array<float, 4U> values{7.0F, -2.0F, 13.0F, 0.5F};
  const GeTensorDesc desc(GeShape({2, 2}), FORMAT_NCHW, DT_FLOAT);
  const GeTensor weight(desc, reinterpret_cast<const uint8_t *>(values.data()), sizeof(values));
  ASSERT_TRUE(AttrUtils::SetTensor(constant->GetOpDesc(), "value", weight));
  ASSERT_EQ(graph->TopologicalSorting(), GRAPH_SUCCESS);
  root = gert::GeModelBuilder(graph).BuildGeRootModel();
  ASSERT_NE(root, nullptr);
  const auto model = root->GetSubgraphInstanceNameToModel().at(subgraph->GetName());
  ASSERT_NE(model, nullptr);
  ASSERT_EQ(model->GetWeightSize(), sizeof(values));
  ASSERT_TRUE(AttrUtils::SetInt(model, ATTR_MODEL_WEIGHT_SIZE, sizeof(values)));
  // This fixture's constant is the third static output; its compiled weight offset must match the buffer.
  constant->GetOpDesc()->SetOutputOffset({0});
  const auto output = subgraph->FindNode("netoutput_sub")->GetOpDesc();
  output->SetInputOffset({48, 0, 0});
  output->SetIsInputConst({false, false, true});
  TensorUtils::SetDataOffset(*output->MutableInputDesc(2U), 0);
}

void CheckKnownSources(const gert::GertModelData &generated) {
  ASSERT_EQ(generated.models.size(), 1U);
  ASSERT_NE(generated.models[0], nullptr);
  ASSERT_NE(generated.models[0]->runtime, nullptr);
  ASSERT_GT(generated.models[0]->runtime->so_artifact.data_size, 0U);
  std::string source;
  for (const auto &artifact : generated.models[0]->runtime->source_artifacts) {
    if (artifact.data != nullptr) {
      source.append(reinterpret_cast<const char *>(artifact.data.get()), artifact.data_size);
    }
  }
  for (const auto *type :
       {"DavinciModelCreate", "DavinciModelGetRunAddress", "DavinciModelExecute", "DavinciModelFinalizer"}) {
    EXPECT_NE(source.find(type), std::string::npos) << type;
  }
}

void RunKnownOutputs(om2_test::GeneratedModelRunner &runner, const float first) {
  std::array<float, 4U> values{first, 2.0F, -3.0F, 4.0F};
  std::array<std::array<float, 4U>, 3U> storage{};
  gert::Tensor input({{2, 2}, {2, 2}}, {FORMAT_NCHW, FORMAT_NCHW, {}}, gert::kOnDeviceHbm, DT_FLOAT, values.data());
  input.SetSize(sizeof(values));
  std::vector<gert::Tensor> output_tensors;
  for (auto &buffer : storage) {
    buffer.fill(-777.0F);
    output_tensors.emplace_back(gert::StorageShape({2, 2}, {2, 2}), gert::StorageFormat(FORMAT_NCHW, FORMAT_NCHW, {}),
                                gert::kOnDeviceHbm, DT_FLOAT, buffer.data());
    output_tensors.back().SetSize(sizeof(buffer));
  }
  gert::Tensor *inputs[]{&input};
  gert::Tensor *outputs[]{&output_tensors[0], &output_tensors[1], &output_tensors[2]};
  GertModelRunConfig run;
  run.input_count = 1U;
  run.input_data = inputs;
  run.output_count = 3U;
  run.output_data = outputs;
  ASSERT_EQ(runner.Run(run), ACL_SUCCESS);
  ASSERT_NE(output_tensors[1].GetData<float>(), nullptr);
  ASSERT_NE(output_tensors[2].GetData<float>(), nullptr);
  const std::array<float, 4U> weights{7.0F, -2.0F, 13.0F, 0.5F};
  // RTS does not compute Conv2d/Relu. The other outputs prove input and weight address binding.
  for (size_t i = 0U; i < values.size(); ++i) {
    EXPECT_FLOAT_EQ(output_tensors[1].GetData<float>()[i], values[i]);
    EXPECT_FLOAT_EQ(output_tensors[2].GetData<float>()[i], weights[i]);
  }
  for (const auto &output : output_tensors) {
    EXPECT_EQ(output.GetStorageShape(), gert::Shape({2, 2}));
    EXPECT_EQ(output.GetDataType(), DT_FLOAT);
    EXPECT_EQ(output.GetSize(), sizeof(values));
  }
}

/**
 * 用例描述：在线 known-shape 子图经真实 OM2 生成、加载、两轮执行和卸载。
 * 预置条件：复用 KnownSubgraph ST 图和 GeModelBuilder；只使用编译模型及 RTS 桩。
 * 测试步骤：在线生成模型 SO，检查创建/地址获取/执行/释放代码；加载后改变输入重复运行。
 * 预期结果：输入直通与常量输出逐元素正确；生命周期成功。RTS 桩不验证 Conv2d 数值。
 */
TEST_F(Om2DynamicResourcesST, OnlineKnownSubgraphPreservesInputAndWeightAcrossRuns) {
  // slow_test_limit = 4000
  gert::GertRuntimeStub stub;
  GeRootModelPtr root;
  ASSERT_NO_FATAL_FAILURE(BuildKnownRoot(root));
  gert::GertModelData generated;
  ASSERT_EQ(Om2PackageHelper().BuildDynamicGertModelData(root, generated), SUCCESS);
  ASSERT_NO_FATAL_FAILURE(CheckKnownSources(generated));
  std::vector<void *> constants(root->GetSubgraphInstanceNameToModel().size(), nullptr);
  ASSERT_NE(generated.constants, nullptr);
  const auto &metadata = generated.models[0]->constants_config;
  ASSERT_NE(metadata, nullptr);
  ASSERT_EQ(metadata->consts.size(), generated.constants->constants_data.size());
  for (size_t i = 0U; i < metadata->consts.size(); ++i) {
    const auto &constant = generated.constants->constants_data[i];
    ASSERT_NE(constant, nullptr);
    ASSERT_NE(metadata->consts[i], nullptr);
    ASSERT_LT(metadata->consts[i]->index, constants.size());
    constants[metadata->consts[i]->index] = const_cast<uint8_t *>(constant->data.get());
  }
  gert::RtSession session;
  struct Cleanup {
    gert::RtSession &session;
    ~Cleanup() {
      session.DestroyResources();
    }
  } cleanup{session};
  om2_test::GeneratedModelRunner runner;
  ASSERT_TRUE(runner.Open(generated.models[0]->runtime->so_artifact)) << runner.Error();
  GertModelDynamicLoadConfig dynamic;
  dynamic.rt_session = &session;
  GertModelLoadConfig load;
  load.dynamic_config = &dynamic;
  load.constants = constants.data();
  ASSERT_EQ(runner.Load(load), ACL_SUCCESS);
  ASSERT_NO_FATAL_FAILURE(RunKnownOutputs(runner, 1.0F));
  ASSERT_NO_FATAL_FAILURE(RunKnownOutputs(runner, 19.0F));
  ASSERT_EQ(runner.Unload(), ACL_SUCCESS);
}

void BadNames(ExecuteGraph &graph) {
  GeTensorDesc desc(GeShape({1}), FORMAT_ND, DT_FLOAT);
  desc.SetName("input");
  ASSERT_TRUE(AttrUtils::SetListTensor(graph, gert::attr::kModelInputs, {GeTensor(desc), GeTensor(desc)}));
}

void BadPorts(ExecuteGraph &graph) {
  const auto affine = ExecuteGraphUtils::FindFirstNodeMatchType(&graph, "Om2ExternalGeneratorAffine");
  const auto input = ExecuteGraphUtils::FindFirstNodeMatchType(&graph, "Data");
  ASSERT_NE(affine, nullptr);
  ASSERT_NE(input, nullptr);
  ASSERT_EQ(affine->GetOpDescPtr()->AddInputDesc(input->GetOpDescPtr()->GetOutputDesc(0)), GRAPH_SUCCESS);
  affine->UpdateDataInNum(4U);
  ASSERT_NE(graph.AddEdge(input, 0, affine, 3), nullptr);
}

void ReversedStages(ExecuteGraph &graph) {
  const auto affine = ExecuteGraphUtils::FindFirstNodeMatchType(&graph, "Om2ExternalGeneratorAffine");
  const auto input = ExecuteGraphUtils::FindFirstNodeMatchType(&graph, "Data");
  ASSERT_NE(affine, nullptr);
  ASSERT_NE(input, nullptr);
  ASSERT_TRUE(AttrUtils::SetStr(input->GetOpDescPtr(), gert::attr::kStage, gert::attr::kStageMain));
  ASSERT_TRUE(AttrUtils::SetStr(affine->GetOpDescPtr(), gert::attr::kStage, gert::attr::kStageInit));
}

/**
 * 用例描述：错误元数据、Kernel 端口及阶段依赖不能发布 OM2 模型。
 * 预置条件：每行重新构造已有合法 affine 执行图，只注入一个错误，不替换校验器。
 * 测试步骤：表驱动修改图，经 BuildGertModelDataFromGraph 公开构建入口尝试生成。
 * 预期结果：每行均命中对应错误诊断，返回失败且输出模型列表为空。
 */
TEST_F(Om2DynamicResourcesST, InvalidGraphsRejectBeforePublishingModel) {
  struct InvalidCase {
    const char *name;
    void (*mutate)(ExecuteGraph &);
    const char *diagnostic;
  };
  const InvalidCase cases[]{
      {"missing_inputs", [](ExecuteGraph &graph) { ASSERT_EQ(graph.DelAttr(gert::attr::kModelInputs), GRAPH_SUCCESS); },
       gert::attr::kModelInputs},
      {"missing_outputs",
       [](ExecuteGraph &graph) { ASSERT_EQ(graph.DelAttr(gert::attr::kModelOutputs), GRAPH_SUCCESS); },
       gert::attr::kModelOutputs},
      {"metadata", BadNames, "Model input description name input is used twice"},
      {"ports", BadPorts, "Failed to collect OM2 kernel affine type Om2ExternalGeneratorAffine"},
      {"stage_dependency", ReversedStages, "produced in later stage main"}};
  gert::GertRuntimeStub stub;
  for (const auto &item : cases) {
    SCOPED_TRACE(item.name);
    auto graph = gert::om2_test::AffineGraph(false);
    ASSERT_NE(graph, nullptr);
    ASSERT_NO_FATAL_FAILURE(item.mutate(*graph));
    auto facade = GraphUtilsEx::ExecuteGraph2Graph(graph);
    ASSERT_NE(facade, nullptr);
    stub.GetSlogStub().Clear();
    gert::GertModelData generated;
    EXPECT_NE(Om2PackageHelper().BuildGertModelDataFromGraph(*facade, generated), SUCCESS);
    EXPECT_TRUE(generated.models.empty());
    EXPECT_NE(stub.GetSlogStub().FindLog(DLOG_ERROR, item.diagnostic), -1);
  }
}

using gert::om2::ChainBindingKind;
using gert::om2::ChainBindingPhase;
using gert::om2::Om2ValueType;

bool ValidEsResourceDescriptors(const gert::ModelDesc *desc, const gert::TypedContinuousVector<aclrtStream> *streams,
                                const gert::TypedContinuousVector<aclrtEvent> *events, const gert::Tensor *first,
                                const gert::Tensor *second, const gert::Tensor *third, bool inspect_public_desc) {
  return desc->GetInputNum() == 3U && desc->GetReusableStreamNum() == 2U &&
         desc->GetAttachedStreamNum() == (inspect_public_desc ? 2U : 0U) &&
         streams->GetSize() == desc->GetReusableStreamNum() + desc->GetAttachedStreamNum() &&
         events->GetSize() == desc->GetReusableEventNum() && first->GetDataType() == DT_INT64 &&
         second->GetDataType() == DT_INT32 && third->GetDataType() == DT_UINT8;
}

graphStatus CheckEsPublicResources(gert::KernelContext *context, const gert::ModelDesc *desc,
                                   const gert::TypedContinuousVector<aclrtStream> *streams, const gert::Tensor *first) {
  const auto *stream = streams->GetData();
  const std::set<aclrtStream> handles(stream, stream + streams->GetSize());
  const auto name_matches = [](const gert::ModelIoDesc *io, const char *name) {
    return io->GetName() != nullptr && std::strcmp(io->GetName(), name) == 0;
  };
  auto *second_output = context->GetInputPointer<gert::Tensor>(11U);
  if (second_output == nullptr || second_output->GetData<int64_t>() == nullptr || desc->GetOutputNum() != 2U ||
      desc->GetReusableEventNum() != 0U || desc->GetReusableNotifyNum() != 0U || handles.size() != 4U ||
      handles.count(nullptr) != 0U || !name_matches(desc->GetInputDesc(0), "z_value") ||
      !name_matches(desc->GetInputDesc(1), "a_factor") || !name_matches(desc->GetInputDesc(2), "m_bias") ||
      !name_matches(desc->GetOutputDesc(0), "z_result") || !name_matches(desc->GetOutputDesc(1), "a_copy")) {
    return GRAPH_FAILED;
  }
  *const_cast<int64_t *>(second_output->GetData<int64_t>()) = *first->GetData<int64_t>();
  return GRAPH_SUCCESS;
}

bool ValidEsStreams(const gert::TypedContinuousVector<aclrtStream> *streams,
                    const gert::TypedContinuousVector<aclrtEvent> *events) {
  const auto *stream = streams->GetData();
  const auto *event = events->GetData();
  if (stream[0] == nullptr || stream[1] == nullptr || stream[0] == stream[1]) {
    return false;
  }
  if (events->GetSize() != 0U && (event[0] == nullptr || aclrtRecordEvent(event[0], stream[1]) != ACL_SUCCESS ||
                                  aclrtStreamWaitEvent(stream[0], event[0]) != ACL_SUCCESS)) {
    return false;
  }
  return true;
}

graphStatus Om2ReadEsResources(gert::KernelContext *context) {
  auto *output = context->GetInputPointer<gert::Tensor>(0U);
  const auto *desc = context->GetInputPointer<gert::ModelDesc>(1U);
  const auto *streams = context->GetInputPointer<gert::TypedContinuousVector<aclrtStream>>(2U);
  const auto *events = context->GetInputPointer<gert::TypedContinuousVector<aclrtEvent>>(3U);
  const auto *first = context->GetInputPointer<gert::Tensor>(4U);
  const auto *second = context->GetInputPointer<gert::Tensor>(5U);
  const auto *third = context->GetInputPointer<gert::Tensor>(6U);
  const auto *other_streams = context->GetInputPointer<gert::TypedContinuousVector<aclrtStream>>(7U);
  const auto *session = context->GetInputPointer<gert::RtSession>(8U);
  const auto *weight = context->GetInputPointer<gert::OuterWeightMem>(9U);
  const auto *opaque_streams = context->GetInputValue<void *>(10U);
  const bool inspect_public_desc = context->GetInputNum() == 12U;
  if (output == nullptr || desc == nullptr || streams == nullptr || events == nullptr || first == nullptr ||
      streams != other_streams || streams != opaque_streams || session == nullptr || weight == nullptr ||
      weight->weight_size != 0U || second == nullptr || third == nullptr ||
      !ValidEsResourceDescriptors(desc, streams, events, first, second, third, inspect_public_desc)) {
    return GRAPH_FAILED;
  }
  if (!ValidEsStreams(streams, events)) {
    return GRAPH_FAILED;
  }
  if (inspect_public_desc && CheckEsPublicResources(context, desc, streams, first) != GRAPH_SUCCESS) {
    return GRAPH_FAILED;
  }
  auto *output_data = const_cast<int64_t *>(output->GetData<int64_t>());
  *output_data = *first->GetData<int64_t>() + 10 * *second->GetData<int32_t>() + 100 * *third->GetData<uint8_t>();
  return GRAPH_SUCCESS;
}
REGISTER_KERNEL(Om2ReadEsResources).RunFunc(Om2ReadEsResources);

class EsResourcesCollector final : public gert::om2::KernelCodeBuilder {
 public:
  graphStatus Collect(const gert::om2::KernelCollectContext &context, gert::om2::KernelProgramInfo &info) override {
    const auto status =
        gert::om2::InitKernelProgramInfo(context, gert::om2::OutputCreatePolicy::kGraphBound, false, info);
    if (status != GRAPH_SUCCESS) {
      return status;
    }
    using Type = Om2ValueType;
    std::vector<gert::om2::KernelInputPortInfo> ports{{"out", Type::kGertTensorPtr},
                                                      {"desc", Type::kGertModelDescPtr},
                                                      {"streams", Type::kTypedContinuousVectorRtStreamTPtr},
                                                      {"events", Type::kVoidPtr},
                                                      {"first", Type::kGertTensorPtr},
                                                      {"second", Type::kGertTensorPtr},
                                                      {"third", Type::kGertTensorPtr},
                                                      {"other_streams", Type::kTypedContinuousVectorRtStreamTPtr},
                                                      {"session", Type::kRtSessionPtr},
                                                      {"weight", Type::kOuterWeightMemPtr},
                                                      {"opaque_streams", Type::kVoidPtr}};
    if (context.input_chain_indices.size() == 12U) {
      ports.push_back({"second_out", Type::kGertTensorPtr});
    }
    const auto port_status = gert::om2::SetInputPortInfos(context, ports, info);
    if (port_status != GRAPH_SUCCESS) {
      return port_status;
    }
    gert::om2::RequestInputBinding(context, 1U, ChainBindingKind::kModelDesc, ChainBindingPhase::kLoad, info);
    gert::om2::RequestInputBinding(context, 2U, ChainBindingKind::kStream, ChainBindingPhase::kLoad, info);
    gert::om2::RequestInputBinding(context, 3U, ChainBindingKind::kRuntimeEvents, ChainBindingPhase::kRun, info);
    gert::om2::RequestInputBinding(context, 7U, ChainBindingKind::kStream, ChainBindingPhase::kLoad, info);
    gert::om2::RequestInputBinding(context, 8U, ChainBindingKind::kRtSession, ChainBindingPhase::kLoad, info);
    gert::om2::RequestInputBinding(context, 9U, ChainBindingKind::kOuterWeightMem, ChainBindingPhase::kLoad, info);
    return GRAPH_SUCCESS;
  }
};
REGISTER_KERNEL_CODE_BUILDER("Om2ReadEsResources", EsResourcesCollector);

void ConnectEsResources(Graph &facade_ref, const std::array<GNode *, 10U> &inputs, bool public_desc) {
  auto *facade = &facade_ref;
  auto output = facade->FindNodeByName("output");
  auto consumer = facade->FindNodeByName("inspect");
  ASSERT_NE(output, nullptr);
  ASSERT_NE(consumer, nullptr);
  std::array<GNode *, 11U> sources{};
  sources[0] = output.get();
  std::copy(inputs.begin(), inputs.end(), sources.begin() + 1);
  for (size_t port = 0U; port < sources.size(); ++port) {
    ASSERT_NE(sources[port], nullptr);
    ASSERT_EQ(facade->AddDataEdge(*sources[port], 0, *consumer, static_cast<int32_t>(port)), GRAPH_SUCCESS);
  }
  std::vector<std::pair<GNode, int32_t>> outputs{{*output, 0}};
  if (public_desc) {
    auto second_output = facade->FindNodeByName("second_output");
    ASSERT_NE(second_output, nullptr);
    ASSERT_EQ(facade->AddDataEdge(*second_output, 0, *consumer, 11), GRAPH_SUCCESS);
    outputs.emplace_back(*second_output, 0);
  }
  ASSERT_EQ(facade->SetOutputs(outputs), GRAPH_SUCCESS);
}

void BuildEsResourcesGraph(Graph &result, bool conflict, bool public_desc) {
  es::EsGraphBuilder builder("es_resources", static_cast<GraphKind>(1));
  auto third = builder.CreateInput(5, "third", DT_UINT8, FORMAT_ND, {1});
  auto model_desc = builder.CreateInput(4, "descriptor");
  auto second = builder.CreateInput(3, "second", DT_INT32, FORMAT_ND, {1});
  auto events = builder.CreateInput(2, "completion_handles");
  auto first = builder.CreateInput(1, "first", DT_INT64, FORMAT_ND, {1});
  auto streams = builder.CreateInput(0, "queues");
  auto other_streams = builder.CreateInput(6, "other_queues");
  ASSERT_NE(other_streams.GetProducer(), nullptr);
  AscendString init_stage(gert::attr::kStageInit);
  ASSERT_EQ(other_streams.GetProducer()->SetAttr(AscendString(gert::attr::kStage), init_stage), GRAPH_SUCCESS);
  auto session = builder.CreateInput(7, "session");
  auto weight = builder.CreateInput(8, "weight");
  auto *facade = builder.GetCGraphBuilder()->GetGraph();
  ASSERT_NE(facade, nullptr);
  auto graph = GraphUtilsEx::Graph2ExecuteGraph(*facade);
  ASSERT_NE(graph, nullptr);
  const GeTensorDesc i64(GeShape({1}), FORMAT_ND, DT_INT64);
  const GeTensorDesc i32(GeShape({1}), FORMAT_ND, DT_INT32);
  const GeTensorDesc u8(GeShape({1}), FORMAT_ND, DT_UINT8);
  ASSERT_NE(gert::om2_test::AddNode(*graph, "output", "OutputData", {}, {i64}), nullptr);
  std::vector<GeTensorDesc> ports{i64, i64, i64, i64, i64, i32, u8, i64, i64, i64, i64};
  if (public_desc) {
    ASSERT_NE(gert::om2_test::AddNode(*graph, "second_output", "OutputData", {}, {i64}), nullptr);
    ports.emplace_back(i64);
  }
  ASSERT_NE(gert::om2_test::AddNode(*graph, "inspect", "Om2ReadEsResources", ports, {}), nullptr);
  const std::array<GNode *, 10U> inputs{
      model_desc.GetProducer(),    streams.GetProducer(),
      events.GetProducer(),        conflict ? streams.GetProducer() : first.GetProducer(),
      second.GetProducer(),        third.GetProducer(),
      other_streams.GetProducer(), session.GetProducer(),
      weight.GetProducer(),        streams.GetProducer()};
  ASSERT_NO_FATAL_FAILURE(ConnectEsResources(*facade, inputs, public_desc));
  result = *facade;
}

void BuildEsResourcesModel(int64_t serialized_count, int64_t explicit_count, gert::GertModelData &generated,
                           bool conflict = false) {
  Graph facade;
  ASSERT_NO_FATAL_FAILURE(BuildEsResourcesGraph(facade, conflict, false));
  auto graph = GraphUtilsEx::Graph2ExecuteGraph(facade);
  ASSERT_NE(graph, nullptr);
  const GeTensorDesc i64(GeShape({1}), FORMAT_ND, DT_INT64);
  const GeTensorDesc i32(GeShape({1}), FORMAT_ND, DT_INT32);
  const GeTensorDesc u8(GeShape({1}), FORMAT_ND, DT_UINT8);
  std::vector<GeTensor> inputs{GeTensor(i64), GeTensor(i32), GeTensor(u8)};
  inputs[0].MutableTensorDesc().SetName("first");
  inputs[1].MutableTensorDesc().SetName("second");
  inputs[2].MutableTensorDesc().SetName("third");
  GeTensor output(i64);
  output.MutableTensorDesc().SetName("output");
  ASSERT_TRUE(AttrUtils::SetListTensor(*graph, gert::attr::kModelInputs, inputs));
  ASSERT_TRUE(AttrUtils::SetListTensor(*graph, gert::attr::kModelOutputs, {output}));
  ASSERT_TRUE(AttrUtils::SetInt(*graph, gert::attr::kModelStreamCount, 2));
  ASSERT_TRUE(AttrUtils::SetInt(*graph, gert::attr::kModelEventCount, serialized_count));
  gert::GertModelDescInfo initial_model;
  ASSERT_EQ(gert::ParseGertModelDesc(*graph, initial_model), GRAPH_SUCCESS);
  ASSERT_EQ(gert::PrepareExecuteGraphMetadata(*graph, initial_model), GRAPH_SUCCESS);
  if (explicit_count < 0) {
    ASSERT_EQ(graph->DelAttr(gert::attr::kModelEventCount), GRAPH_SUCCESS);
  } else {
    ASSERT_TRUE(AttrUtils::SetInt(*graph, gert::attr::kModelEventCount, explicit_count));
  }
  const auto status = Om2PackageHelper().BuildGertModelDataFromGraph(facade, generated);
  if (conflict) {
    EXPECT_NE(status, SUCCESS);
    EXPECT_TRUE(generated.models.empty());
  } else {
    ASSERT_EQ(status, SUCCESS);
    ASSERT_EQ(generated.models.size(), 1U);
    ASSERT_NE(generated.models[0], nullptr);
    ASSERT_NE(generated.models[0]->runtime, nullptr);
  }
}

class EsResourceAclStub final : public gert::AclRuntimeStubImpl {
 public:
  aclError aclrtCreateEventExWithFlag(aclrtEvent *event, uint32_t flag) override {
    EXPECT_EQ(flag, ACL_EVENT_SYNC);
    ++attempts;
    if (attempts == fail_on) {
      return ACL_ERROR_FAILURE;
    }
    *event = static_cast<void *>(&handles.at(created.size()));
    created.push_back(*event);
    return ACL_SUCCESS;
  }

  aclError aclrtRecordEvent(aclrtEvent event, aclrtStream stream) override {
    EXPECT_EQ(event, created.front());
    recorded.push_back(stream);
    return ACL_SUCCESS;
  }

  aclError aclrtStreamWaitEvent(aclrtStream stream, aclrtEvent event) override {
    EXPECT_EQ(event, created.front());
    waited.push_back(stream);
    return ACL_SUCCESS;
  }

  aclError aclrtSynchronizeStream(aclrtStream stream) override {
    synchronized.push_back(stream);
    return ACL_SUCCESS;
  }

  aclError aclrtDestroyEvent(aclrtEvent event) override {
    if (!recorded.empty()) {
      EXPECT_NE(std::find(synchronized.begin(), synchronized.end(), recorded.front()), synchronized.end());
    }
    destroyed.push_back(event);
    return ACL_SUCCESS;
  }

  size_t fail_on = 0U;
  size_t attempts = 0U;
  std::array<int, 2U> handles{};
  std::vector<aclrtEvent> created;
  std::vector<aclrtEvent> destroyed;
  std::vector<aclrtStream> recorded;
  std::vector<aclrtStream> waited;
  std::vector<aclrtStream> synchronized;
};

void RunEsResources(om2_test::GeneratedModelRunner &runner, aclrtStream stream, int64_t seed,
                    bool expect_second_output = false) {
  int32_t second = -3;
  uint8_t third = 11;
  int64_t result = -1;
  int64_t second_result = -1;
  const gert::StorageShape shape({1}, {1});
  const gert::StorageFormat format(FORMAT_ND, FORMAT_ND, {});
  gert::Tensor first(shape, format, gert::kOnHost, DT_INT64, &seed);
  gert::Tensor middle(shape, format, gert::kOnHost, DT_INT32, &second);
  gert::Tensor last(shape, format, gert::kOnHost, DT_UINT8, &third);
  gert::Tensor output(shape, format, gert::kOnHost, DT_INT64, &result);
  gert::Tensor second_output(shape, format, gert::kOnHost, DT_INT64, &second_result);
  second_output.SetSize(sizeof(second_result));
  first.SetSize(sizeof(seed));
  middle.SetSize(sizeof(second));
  last.SetSize(sizeof(third));
  output.SetSize(sizeof(result));
  gert::Tensor *inputs[]{&first, &middle, &last};
  gert::Tensor *outputs[]{&output, &second_output};
  GertModelRunConfig run;
  run.input_count = 3U;
  run.input_data = inputs;
  run.output_count = expect_second_output ? 2U : 1U;
  run.output_data = outputs;
  ASSERT_EQ(runner.RunAsync(stream, run), ACL_SUCCESS);
  ASSERT_EQ(aclrtSynchronizeStream(stream), ACL_SUCCESS);
  EXPECT_EQ(result, seed + 1070);
  if (expect_second_output) {
    EXPECT_EQ(second_result, seed);
  }
}

void CheckEsResourceInputs(const gert::GertModelData &generated) {
  const auto &inputs = generated.models[0]->model_meta->input_desc;
  ASSERT_EQ(inputs.size(), 3U);
  EXPECT_STREQ(inputs[0].name.get(), "first");
  EXPECT_STREQ(inputs[1].name.get(), "second");
  EXPECT_STREQ(inputs[2].name.get(), "third");
  EXPECT_EQ(inputs[0].data_type, DT_INT64);
  EXPECT_EQ(inputs[1].data_type, DT_INT32);
  EXPECT_EQ(inputs[2].data_type, DT_UINT8);
}

/**
 * 用例描述：交错的公开 Data 绑定运行和加载资源，两个独立 Feed 共享同一 streams 集合。
 * 预置条件：真实 OM2 生成模型；ACL 事件和流使用桩记录调用。
 * 测试步骤：检查当前事件属性覆盖和删除后恢复默认值，两轮执行后卸载，再注入第二次事件创建失败。
 * 预期结果：实参 dtype 和输出正确，主辅流分别 wait/record，句柄仅在 load 创建且逐个回收。
 */
TEST_F(Om2DynamicResourcesST, EsResourceBindingsAndEventLifecycleWithAclStub) {
  // slow_test_limit = 7000
  const struct {
    int64_t old_count;
    int64_t explicit_count;
    size_t expected;
  } cases[]{{0, 2, 2U}, {2, 0, 0U}, {2, -1, 0U}};
  for (const auto &item : cases) {
    SCOPED_TRACE(item.explicit_count);
    auto runtime = std::make_unique<EsResourceAclStub>();
    auto *observed = runtime.get();
    gert::GertRuntimeStub stub(std::make_unique<gert::RuntimeStubImpl>(), true, std::move(runtime));
    gert::GertModelData generated;
    ASSERT_NO_FATAL_FAILURE(BuildEsResourcesModel(item.old_count, item.explicit_count, generated));
    ASSERT_NO_FATAL_FAILURE(CheckEsResourceInputs(generated));
    gert::RtSession session;
    om2_test::GeneratedModelRunner runner;
    ASSERT_TRUE(runner.Open(generated.models[0]->runtime->so_artifact)) << runner.Error();
    GertModelDynamicLoadConfig dynamic;
    dynamic.rt_session = &session;
    GertModelLoadConfig load;
    load.dynamic_config = &dynamic;
    ASSERT_EQ(runner.Load(load), ACL_SUCCESS);
    ASSERT_EQ(observed->created.size(), item.expected);
    int borrowed_stream = 0;
    ASSERT_NO_FATAL_FAILURE(RunEsResources(runner, &borrowed_stream, 5));
    ASSERT_NO_FATAL_FAILURE(RunEsResources(runner, &borrowed_stream, 73));
    EXPECT_EQ(observed->attempts, item.expected);
    EXPECT_EQ(observed->recorded.size(), item.expected == 0U ? 0U : 2U);
    for (const auto stream : observed->waited) {
      EXPECT_EQ(stream, &borrowed_stream);
    }
    EXPECT_EQ(observed->waited.size(), observed->recorded.size());
    ASSERT_EQ(runner.Unload(), ACL_SUCCESS);
    EXPECT_EQ(observed->destroyed, observed->created);
    if (item.explicit_count == 2) {
      observed->created.clear();
      observed->destroyed.clear();
      observed->recorded.clear();
      observed->fail_on = observed->attempts + 2U;
      EXPECT_NE(runner.Load(load), ACL_SUCCESS);
      ASSERT_EQ(observed->created.size(), 1U);
      EXPECT_EQ(observed->destroyed, observed->created);
    }
  }
}

/**
 * 用例描述：同一个 Data 同时流向 streams 和普通 Tensor 端口时拒绝编译。
 * 预置条件：除冲突边外使用与资源生命周期用例相同的公开 ES 图。
 * 测试步骤：调用公开模型生成入口。
 * 预期结果：编译返回失败，模型列表为空。
 */
TEST_F(Om2DynamicResourcesST, EsResourceAndUserSemanticsConflictRejectsGraph) {
  gert::GertRuntimeStub stub;
  gert::GertModelData generated;
  ASSERT_NO_FATAL_FAILURE(BuildEsResourcesModel(0, 0, generated, true));
}
class Om2PublicModelDescST : public Om2DynamicResourcesST, public testing::WithParamInterface<bool> {
 protected:
  void SetUp() override {
    Om2DynamicResourcesST::SetUp();
    const std::map<AscendString, AscendString> options;
    ASSERT_EQ(GEInitializeV2(options), SUCCESS);
    session = std::make_unique<GeSession>(options);
  }

  void TearDown() override {
    session.reset();
    EXPECT_EQ(GEFinalizeV2(), SUCCESS);
    Om2DynamicResourcesST::TearDown();
  }

  std::unique_ptr<GeSession> session;
  om2_test::ScopedEnvVar online{"ENABLE_RUNTIME_OM2", "1"};
  gert::GertRuntimeStub stub;
};

template <typename T>
void SetModelGraphAttr(Graph &graph, const char *key, const T &value) {
  AttrValue attr;
  ASSERT_EQ(attr.SetAttrValue(value), GRAPH_SUCCESS);
  ASSERT_EQ(graph.SetAttr(AscendString(key), attr), GRAPH_SUCCESS);
}

Tensor NamedModelTensor(const char *name, DataType dtype) {
  TensorDesc desc(Shape({1}), FORMAT_ND, dtype);
  desc.SetName(name);
  return Tensor(desc);
}

void CheckPublicIoNames(const gert::GertModelData &data) {
  ASSERT_EQ(data.models.size(), 1U);
  const auto &meta = *data.models[0]->model_meta;
  ASSERT_EQ(meta.input_desc.size(), 3U);
  ASSERT_EQ(meta.output_desc.size(), 2U);
  EXPECT_STREQ(meta.input_desc[0].name.get(), "z_value");
  EXPECT_STREQ(meta.input_desc[1].name.get(), "a_factor");
  EXPECT_STREQ(meta.input_desc[2].name.get(), "m_bias");
  EXPECT_STREQ(meta.output_desc[0].name.get(), "z_result");
  EXPECT_STREQ(meta.output_desc[1].name.get(), "a_copy");
}

TEST_P(Om2PublicModelDescST, PublicTensorNamesAndAttachedStreamsSurviveCompileAndFileLoad) {
  // slow_test_limit = 3000
  Graph graph;
  ASSERT_NO_FATAL_FAILURE(BuildEsResourcesGraph(graph, false, true));
  ASSERT_NO_FATAL_FAILURE(SetModelGraphAttr(
      graph, gert::attr::kModelInputs,
      std::vector<Tensor>{NamedModelTensor("z_value", DT_INT64), NamedModelTensor("a_factor", DT_INT32),
                          NamedModelTensor("m_bias", DT_UINT8)}));
  ASSERT_NO_FATAL_FAILURE(SetModelGraphAttr(
      graph, gert::attr::kModelOutputs,
      std::vector<Tensor>{NamedModelTensor("z_result", DT_INT64), NamedModelTensor("a_copy", DT_INT64)}));
  ASSERT_NO_FATAL_FAILURE(SetModelGraphAttr(graph, gert::attr::kModelStreamCount, int64_t{2}));
  if (GetParam()) {
    ASSERT_NO_FATAL_FAILURE(SetModelGraphAttr(graph, gert::attr::kModelEventCount, int64_t{0}));
    ASSERT_NO_FATAL_FAILURE(SetModelGraphAttr(graph, gert::attr::kModelNotifyCount, int64_t{0}));
  }
  ASSERT_NO_FATAL_FAILURE(SetModelGraphAttr(graph, gert::attr::kModelAttachedStreamCount, int64_t{2}));

  constexpr uint32_t id = 725U;
  const std::map<AscendString, AscendString> options{
      {"ge.buildConfig", "make -s CXXFLAGS='-std=c++17 -fPIC -O0 -D_GLIBCXX_USE_CXX11_ABI=0'"}};
  ASSERT_EQ(session->AddGraph(id, graph, options), SUCCESS);
  ASSERT_EQ(session->CompileGraph(id), SUCCESS);
  GraphNodePtr compiled;
  ASSERT_EQ(session->impl_->GetInnerSession()->getGraphManagerObj().GetGraphNode(id, compiled), SUCCESS);
  const auto data = compiled->GetGeRootModel()->GetOm2ModelData();
  ASSERT_NE(data, nullptr);
  ASSERT_NO_FATAL_FAILURE(CheckPublicIoNames(*data));
  ModelBufferData archive;
  ASSERT_EQ(gert::SerializeGertModelData(*data, archive, false), SUCCESS);
  const auto path = std::filesystem::path(std::getenv("ASCEND_WORK_PATH")) /
                    (GetParam() ? "public_model_desc_explicit_counts" : "public_model_desc");
  ASSERT_EQ(aclgrphSaveModel(path.c_str(), archive), SUCCESS);
  ASSERT_EQ(session->RemoveGraph(id), SUCCESS);

  ModelData loaded;
  ASSERT_EQ(gert::LoadOm2DataFromFile(path.string() + ".om2", loaded), SUCCESS);
  const std::unique_ptr<const uint8_t[]> bytes(static_cast<const uint8_t *>(loaded.model_data));
  gert::GertModelData restored;
  ASSERT_EQ(gert::DeserializeGertModelData(bytes.get(), loaded.model_len, &restored), SUCCESS);
  ASSERT_NO_FATAL_FAILURE(CheckPublicIoNames(restored));
  om2_test::GeneratedModelRunner runner;
  ASSERT_TRUE(runner.Open(restored.models[0]->runtime->so_artifact)) << runner.Error();
  gert::RtSession rt_session;
  GertModelDynamicLoadConfig dynamic;
  dynamic.rt_session = &rt_session;
  GertModelLoadConfig load;
  load.dynamic_config = &dynamic;
  ASSERT_EQ(runner.Load(load), ACL_SUCCESS);
  int borrowed_stream = 0;
  ASSERT_NO_FATAL_FAILURE(RunEsResources(runner, &borrowed_stream, 17, true));
  EXPECT_EQ(runner.Unload(), ACL_SUCCESS);
}
INSTANTIATE_TEST_SUITE_P(ExplicitAndDefaultCounts, Om2PublicModelDescST, testing::Bool());
}  // namespace
}  // namespace ge
