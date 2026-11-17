/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <gtest/gtest.h>
#include <map>
#include <queue>
#include <set>
#include "faker/fake_value.h"
#include "common/share_graph.h"
#include "faker/global_data_faker.h"
#include "framework/runtime/model_v2_executor.h"
#include "framework/runtime/executor_option/multi_thread_executor_option.h"
#include "core/executor/multi_thread_topological/executor/schedule/producer/task_producer_factory.h"
#include "core/executor/multi_thread_topological/executor/schedule/config/task_scheduler_config.h"
#include "core/executor/multi_thread_topological/execution_data/multi_thread_execution_data.h"
#include "common/bg_test.h"
#include "acl/acl_rt.h"
#include "kernel/memory/caching_mem_allocator.h"
#include "stub/gert_runtime_stub.h"
#include "op_impl/data_flow_op_impl.h"
#include "lowering/model_converter.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/execute_graph_utils.h"
#include "graph/utils/fast_node_utils.h"
#include "core/builder/node_types.h"
#include "register/op_tiling/op_tiling_constants.h"
#include "register/kernel_registry.h"
#include "faker/magic_ops.h"
#include "check/executor_statistician.h"
#include "depends/profiler/src/profiling_test_util.h"
#include "common/global_variables/diagnose_switch.h"
#include "kernel/memory/memory_kernel.h"

using namespace ge;
namespace gert {
namespace {
std::mutex g_fake_mtx;
uint32_t g_fake_flag = 0U;
graphStatus CalcTensorSizeFromStorageFailedFake(gert::KernelContext *context) {
  std::unique_lock<std::mutex> lk(g_fake_mtx);
  if (g_fake_flag++ == 0U) {
    return GRAPH_FAILED;
  }
  return GRAPH_SUCCESS;
}
}  // namespace
class MultiThreadExecutorE2ESystemTest : public bg::BgTest {
 protected:
  void SetUp() override {
    bg::BgTest::SetUp();
    aclrtSetDevice(0);
    constexpr int32_t kEnvNoOverwrite = 0;
    int32_t mmRet = 0;
    MM_SYS_SET_ENV(MM_ENV_MAX_RUNTIME_CORE_NUMBER, "3", kEnvNoOverwrite, mmRet);
    (void)mmRet;
  }
  void TearDown() override {
    Test::TearDown();
    int32_t mmRet = 0;
    MM_SYS_UNSET_ENV(MM_ENV_MAX_RUNTIME_CORE_NUMBER, mmRet);
    (void)mmRet;
    while (bg::ValueHolder::PopGraphFrame() != nullptr) {
    }
  }
};

const std::string TransDataStubName = "TransDataStubBin";
const std::string TransData13StubName = "TransData17StubBin";
const std::string DynamicAtomicStubName = "DynamicAtomicBin";
const std::string DynamicRnnv3StubName = "DynamicRNNV3StubBin";
const std::string AddStubName = "AddStubBin";
const std::string MulStubName = "MulStubBin";
const std::string ReduceSumStubName = "ReduceSumStubBin";
const size_t kThreadNum = kLeastThreadNumber + 1;

void TaskParallelScheduleBy(const TaskProducerType &producer_type) {
  auto graph = ShareGraph::LstmpGraph();
  graph->TopologicalSorting();
  GeModelBuilder builder(graph);
  auto ge_root_model =
      builder
          .AddTaskDef("TransData",
                      AiCoreTaskDefFaker(TransDataStubName).AtomicStubNum(DynamicAtomicStubName).WithHandle())
          .AddTaskDef("DynamicRNNV3", AiCoreTaskDefFaker(DynamicRnnv3StubName))
          .BuildGeRootModel();

  bg::ValueHolder::PopGraphFrame();  // 不需要BgTest自带的Frame
  auto exe_graph = ModelConverter().ConvertGeModelToExecuteGraph(ge_root_model);
  ASSERT_NE(exe_graph, nullptr);
  ASSERT_EQ(3, exe_graph->GetDirectNodesSize());

  GertRuntimeStub runtime_stub;
  runtime_stub.GetKernelStub().StubTiling();

  TaskProducerFactory::GetInstance().SetProducerType(producer_type);
  ASSERT_EQ(TaskProducerFactory::GetInstance().GetProducerType(), producer_type);
  auto option = MultiThreadExecutorOption(kThreadNum);
  auto model_executor = ModelV2Executor::Create(exe_graph, option, ge_root_model);
  ASSERT_NE(model_executor, nullptr);

  auto allocator = memory::CachingMemAllocator::GetAllocator();
  auto mem_block = allocator->Malloc(2048 * 2);
  memset_s(const_cast<void *>(mem_block->GetAddr()), mem_block->GetSize(), 0, 2048 * 2);

  auto multi_thread_ed = reinterpret_cast<const MultiThreadExecutionData *>(
      model_executor->GetExeGraphExecutor(SubExeGraphType::kMainExeGraph)->GetExecutionData());
  multi_thread_ed->scheduler->DumpBrief();
  ASSERT_EQ(model_executor->Load(), ge::GRAPH_SUCCESS);
  auto outputs = FakeTensors({2048}, 3, const_cast<void *>(mem_block->GetAddr()));

  auto i0 = FakeValue<Tensor>(Tensor{{{2048}, {2048}},
                                     {ge::FORMAT_ND, ge::FORMAT_ND, {}},
                                     kOnHost,
                                     ge::DT_FLOAT16,
                                     const_cast<void *>(mem_block->GetAddr())});
  auto i1 = FakeValue<Tensor>(Tensor{{{2048}, {2048}},
                                     {ge::FORMAT_ND, ge::FORMAT_ND, {}},
                                     kOnHost,
                                     ge::DT_FLOAT16,
                                     const_cast<void *>(mem_block->GetAddr())});
  auto i2 = FakeValue<Tensor>(Tensor{{{2048}, {2048}},
                                     {ge::FORMAT_ND, ge::FORMAT_ND, {}},
                                     kOnHost,
                                     ge::DT_FLOAT16,
                                     const_cast<void *>(mem_block->GetAddr())});

  rtStream_t stream;
  ASSERT_EQ(aclrtCreateStreamWithConfig(&stream, static_cast<uint32_t>(RT_STREAM_PRIORITY_DEFAULT), 0), RT_ERROR_NONE);
  auto i3 = FakeValue<uint64_t>(reinterpret_cast<uint64_t>(stream));
  ASSERT_EQ(model_executor->Execute({i3.value},
                                    std::vector<Tensor *>({i0.holder.get(), i1.holder.get(), i2.holder.get()}).data(),
                                    3, reinterpret_cast<Tensor **>(outputs.GetAddrList()), outputs.size()),
            ge::GRAPH_SUCCESS);
  ASSERT_EQ(model_executor->Execute({i3.value},
                                    std::vector<Tensor *>({i0.holder.get(), i1.holder.get(), i2.holder.get()}).data(),
                                    3, reinterpret_cast<Tensor **>(outputs.GetAddrList()), outputs.size()),
            ge::GRAPH_SUCCESS);
  ASSERT_EQ(model_executor->UnLoad(), ge::GRAPH_SUCCESS);
  ASSERT_TRUE(runtime_stub.CheckLaunchWhenStubTiling());
  multi_thread_ed->scheduler->Dump();
  aclrtDestroyStream(stream);
  mem_block->Free();
}

void RunIfGraph(TensorHolder &pred_tensor, bool expect_branch, const TaskProducerType &producer_type) {
  auto compute_graph = ShareGraph::IfGraph2();
  ASSERT_NE(compute_graph, nullptr);
  compute_graph->TopologicalSorting();
  auto pred_data_desc = compute_graph->FindNode("pred")->GetOpDesc()->MutableOutputDesc(0);
  pred_data_desc->SetShape(ge::GeShape());
  pred_data_desc->SetOriginShape(ge::GeShape());
  GeModelBuilder builder(compute_graph);
  auto ge_root_model = builder.BuildGeRootModel();

  GertRuntimeStub rtstub;
  rtstub.StubByNodeTypes({"Shape", "Rank"});

  auto exe_graph = ModelConverter().ConvertGeModelToExecuteGraph(ge_root_model);
  ASSERT_NE(exe_graph, nullptr);
  TaskProducerFactory::GetInstance().SetProducerType(producer_type);
  ASSERT_EQ(TaskProducerFactory::GetInstance().GetProducerType(), producer_type);
  auto option = MultiThreadExecutorOption(kThreadNum);
  auto model_executor = ModelV2Executor::Create(exe_graph, option, ge_root_model);
  ASSERT_NE(model_executor, nullptr);

  EXPECT_EQ(model_executor->Load(), ge::GRAPH_SUCCESS);

  auto input_holder = TensorFaker().Placement(kOnDeviceHbm).DataType(ge::DT_FLOAT).Shape({8, 3, 224, 224}).Build();
  std::vector<Tensor *> inputs{pred_tensor.GetTensor(), input_holder.GetTensor()};
  auto output_holder = TensorFaker().Placement(kOnHost).DataType(ge::DT_INT64).Shape({8}).Build();
  std::vector<Tensor *> outputs{output_holder.GetTensor()};
  rtStream_t stream;
  ASSERT_EQ(aclrtCreateStreamWithConfig(&stream, static_cast<uint32_t>(RT_STREAM_PRIORITY_DEFAULT), 0), RT_ERROR_NONE);
  auto stream_value = FakeValue<uint64_t>(reinterpret_cast<uint64_t>(stream));

  auto output_shape = expect_branch ? Shape({4}) : Shape({});
  auto output_data = expect_branch ? Shape({8, 3, 224, 224}) : Shape({4});

  ASSERT_EQ(model_executor->Execute({stream_value.value}, inputs.data(), inputs.size(), outputs.data(), outputs.size()),
            ge::GRAPH_SUCCESS);
  EXPECT_EQ(output_holder.GetTensor()->GetShape().GetStorageShape(), output_shape);
  EXPECT_EQ(output_holder.GetTensor()->GetShape().GetOriginShape(), output_shape);
  EXPECT_EQ(memcmp(output_holder.GetTensor()->GetAddr(), &output_data[0], output_data.GetDimNum() * 8), 0);

  ASSERT_EQ(model_executor->Execute({stream_value.value}, inputs.data(), inputs.size(), outputs.data(), outputs.size()),
            ge::GRAPH_SUCCESS);
  EXPECT_EQ(output_holder.GetTensor()->GetShape().GetStorageShape(), output_shape);
  EXPECT_EQ(output_holder.GetTensor()->GetShape().GetOriginShape(), output_shape);
  EXPECT_EQ(memcmp(output_holder.GetTensor()->GetAddr(), &output_data[0], output_data.GetDimNum() * 8), 0);

  ASSERT_EQ(model_executor->UnLoad(), ge::GRAPH_SUCCESS);
  aclrtDestroyStream(stream);
}

void RunWhileGraph(const TaskProducerType &producer_type) {
  GertRuntimeStub stub;
  MagicOpFakerBuilder("LessThan_5")
      .OutputPlacement(kOnHost)
      .KernelFunc([](KernelContext *ctx) {
        auto input = ctx->GetInputPointer<GertTensorData>(kFkiStart + 1)->GetAddr();
        auto output = ctx->GetOutputPointer<GertTensorData>(1)->GetAddr();
        *static_cast<bool *>(output) = (*static_cast<int32_t *>(input) < 5);
        return ge::GRAPH_SUCCESS;
      })
      .Build(stub);

  MagicOpFakerBuilder("Add_1")
      .KernelFunc([](KernelContext *ctx) {
        auto input = ctx->GetInputPointer<GertTensorData>(kFkiStart + 1);
        auto tensor_data = ctx->GetOutputPointer<GertTensorData>(1);
        *static_cast<int32_t *>(tensor_data->GetAddr()) = *static_cast<int32_t *>(input->GetAddr()) + 1;
        tensor_data->SetPlacement(input->GetPlacement());
        tensor_data->SetSize(input->GetSize());
        return ge::GRAPH_SUCCESS;
      })
      .Build(stub);

  for (auto &graph : {ShareGraph::WhileGraph2()}) {
    graph->TopologicalSorting();
    GeModelBuilder builder(graph);
    auto ge_root_model = builder.BuildGeRootModel();

    bg::ValueHolder::PopGraphFrame();  // 不需要BgTest自带的Frame
    auto exe_graph = ModelConverter().ConvertGeModelToExecuteGraph(ge_root_model);
    ASSERT_NE(exe_graph, nullptr);
    ASSERT_EQ(3, exe_graph->GetDirectNodesSize());

    TaskProducerFactory::GetInstance().SetProducerType(producer_type);
    ASSERT_EQ(TaskProducerFactory::GetInstance().GetProducerType(), producer_type);
    auto option = MultiThreadExecutorOption(kThreadNum);
    auto model_executor = ModelV2Executor::Create(exe_graph, option, ge_root_model);
    ASSERT_NE(model_executor, nullptr);

    ASSERT_EQ(model_executor->Load(), ge::GRAPH_SUCCESS);
    auto outputs = FakeTensors({1, 1, 224, 224}, 1);

    rtStream_t stream;
    ASSERT_EQ(aclrtCreateStreamWithConfig(&stream, static_cast<uint32_t>(RT_STREAM_PRIORITY_DEFAULT), 0),
              RT_ERROR_NONE);
    auto i1 = FakeValue<uint64_t>(reinterpret_cast<uint64_t>(stream));

    auto inputs = FakeTensors({1, 1, 224, 224}, 1);
    *static_cast<int32_t *>(inputs.data()[0].GetAddr()) = 0;

    ASSERT_EQ(model_executor->Execute({i1.value}, inputs.GetTensorList(), inputs.size(), outputs.GetTensorList(),
                                      outputs.size()),
              ge::GRAPH_SUCCESS);
    // Check output value
    EXPECT_EQ(*static_cast<int32_t *>(static_cast<gert::Tensor *>(outputs.GetAddrList()[0])->GetAddr()), 5);

    *static_cast<int32_t *>(inputs.data()[0].GetAddr()) = 8;
    ASSERT_EQ(model_executor->Execute({i1.value}, inputs.GetTensorList(), inputs.size(), outputs.GetTensorList(),
                                      outputs.size()),
              ge::GRAPH_SUCCESS);
    EXPECT_EQ(*static_cast<int32_t *>(static_cast<gert::Tensor *>(outputs.GetAddrList()[0])->GetAddr()), 8);

    ASSERT_EQ(model_executor->UnLoad(), ge::GRAPH_SUCCESS);
    aclrtDestroyStream(stream);
  }
}

void RunCaseGraph(TensorHolder &index_tensor, const TaskProducerType &producer_type) {
  auto compute_graph = ShareGraph::CaseGraph();
  ASSERT_NE(compute_graph, nullptr);
  compute_graph->TopologicalSorting();
  auto index_data_desc = compute_graph->FindNode("index")->GetOpDesc()->MutableOutputDesc(0);
  index_data_desc->SetShape(ge::GeShape());
  index_data_desc->SetOriginShape(ge::GeShape());
  GeModelBuilder builder(compute_graph);
  auto ge_root_model = builder.BuildGeRootModel();

  GertRuntimeStub rtstub;
  rtstub.StubByNodeTypes({"Shape", "Rank", "Size"});

  auto exe_graph = ModelConverter().ConvertGeModelToExecuteGraph(ge_root_model);
  ASSERT_NE(exe_graph, nullptr);
  TaskProducerFactory::GetInstance().SetProducerType(producer_type);
  ASSERT_EQ(TaskProducerFactory::GetInstance().GetProducerType(), producer_type);
  auto option = MultiThreadExecutorOption(kThreadNum);
  auto model_executor = ModelV2Executor::Create(exe_graph, option, ge_root_model);
  ASSERT_NE(model_executor, nullptr);

  EXPECT_EQ(model_executor->Load(), ge::GRAPH_SUCCESS);

  auto input_holder = TensorFaker().Placement(kOnDeviceHbm).DataType(ge::DT_FLOAT).Shape({8, 3, 224, 224}).Build();
  std::vector<Tensor *> inputs{index_tensor.GetTensor(), input_holder.GetTensor()};
  auto output_holder = TensorFaker().Placement(kOnHost).DataType(ge::DT_INT64).Shape({8}).Build();
  std::vector<Tensor *> outputs{output_holder.GetTensor()};
  rtStream_t stream;
  ASSERT_EQ(aclrtCreateStreamWithConfig(&stream, static_cast<uint32_t>(RT_STREAM_PRIORITY_DEFAULT), 0), RT_ERROR_NONE);
  auto stream_value = FakeValue<uint64_t>(reinterpret_cast<uint64_t>(stream));

  ASSERT_EQ(model_executor->Execute({stream_value.value}, inputs.data(), inputs.size(), outputs.data(), outputs.size()),
            ge::GRAPH_SUCCESS);

  ASSERT_EQ(model_executor->Execute({stream_value.value}, inputs.data(), inputs.size(), outputs.data(), outputs.size()),
            ge::GRAPH_SUCCESS);

  ASSERT_EQ(model_executor->UnLoad(), ge::GRAPH_SUCCESS);
  aclrtDestroyStream(stream);
}

void RunGraphFailThenSuccess(const TaskProducerType &producer_type) {
  auto graph = ShareGraph::IfCondByShapeGraph();
  graph->TopologicalSorting();
  auto pred_data_desc = graph->FindNode("pred")->GetOpDesc()->MutableOutputDesc(0);
  pred_data_desc->SetShape(ge::GeShape());
  pred_data_desc->SetOriginShape(ge::GeShape());
  auto input_data_desc = graph->FindNode("input")->GetOpDesc()->MutableOutputDesc(0);
  input_data_desc->SetShape(ge::GeShape({2, 3, 4, 6}));
  input_data_desc->SetOriginShape(ge::GeShape({2, 3, 4, 6}));
  const char *const Cast = "Cast";
  auto ge_root_model = GeModelBuilder(graph)
                           .AddTaskDef("Add", AiCoreTaskDefFaker("AddStubBin").WithHandle())
                           .AddTaskDef(Cast, AiCoreTaskDefFaker(Cast).WithHandle())
                           .FakeTbeBin({Cast})
                           .BuildGeRootModel();

  auto exe_graph = ModelConverter().ConvertGeModelToExecuteGraph(ge_root_model);
  ASSERT_NE(exe_graph, nullptr);

  GertRuntimeStub runtime_stub;
  runtime_stub.GetKernelStub().SetUp("CalcTensorSizeFromStorage", CalcTensorSizeFromStorageFailedFake);

  TaskProducerFactory::GetInstance().SetProducerType(producer_type);
  ASSERT_EQ(TaskProducerFactory::GetInstance().GetProducerType(), producer_type);
  auto option = MultiThreadExecutorOption(kThreadNum);
  auto model_executor = ModelV2Executor::Create(exe_graph, option, ge_root_model);
  ASSERT_NE(model_executor, nullptr);

  auto ess = StartExecutorStatistician(model_executor);
  EXPECT_EQ(model_executor->Load(), ge::GRAPH_SUCCESS);

  auto output_holder =
      TensorFaker().Placement(kOnDeviceHbm).DataType(ge::DT_FLOAT).Format(ge::FORMAT_ND).Shape({2, 3, 4, 6}).Build();
  std::vector<Tensor *> outputs{output_holder.GetTensor()};

  auto i0 = TensorFaker().Placement(kOnDeviceHbm).DataType(ge::DT_INT32).Value<int32_t>({1}).Build();
  auto i1 = TensorFaker().Placement(kOnDeviceHbm).DataType(ge::DT_FLOAT).Shape({2, 3, 4, 6}).Build();
  auto inputs0 = std::vector<Tensor *>({i0.GetTensor(), i1.GetTensor()});

  rtStream_t stream;
  ASSERT_EQ(aclrtCreateStreamWithConfig(&stream, static_cast<uint32_t>(RT_STREAM_PRIORITY_DEFAULT), 0), RT_ERROR_NONE);
  auto i3 = FakeValue<uint64_t>(reinterpret_cast<uint64_t>(stream));

  // 第一次执行失败
  g_fake_flag = 0U;
  ess->Clear();
  ASSERT_NE(model_executor->Execute({i3.value}, inputs0.data(), inputs0.size(), outputs.data(), outputs.size()),
            ge::GRAPH_SUCCESS);
  // 执行失败，和失败结点关联的后续launch结点都不会执行
  ASSERT_EQ(ess->GetExecuteCountByNodeNameAndKernelType("add0", "LaunchKernelV2"), 0);

  // 第二次执行成功
  ess->Clear();
  ASSERT_EQ(model_executor->Execute({i3.value}, inputs0.data(), inputs0.size(), outputs.data(), outputs.size()),
            ge::GRAPH_SUCCESS);
  // 执行成功，所有launch结点都正常执行
  ASSERT_EQ(ess->GetExecuteCountByNodeNameAndKernelType("add0", "LaunchKernelV2"), 1);

  ASSERT_EQ(model_executor->UnLoad(), ge::GRAPH_SUCCESS);

  auto new_model_executor = ModelV2Executor::Create(exe_graph, option, ge_root_model);
  ASSERT_NE(new_model_executor, nullptr);
  EXPECT_EQ(new_model_executor->Load(), ge::GRAPH_SUCCESS);

  g_fake_flag = 0U;
  ASSERT_NE(new_model_executor->Execute({i3.value}, inputs0.data(), inputs0.size(), outputs.data(), outputs.size()),
            ge::GRAPH_SUCCESS);
  ASSERT_EQ(new_model_executor->Execute({i3.value}, inputs0.data(), inputs0.size(), outputs.data(), outputs.size()),
            ge::GRAPH_SUCCESS);
  ASSERT_EQ(new_model_executor->UnLoad(), ge::GRAPH_SUCCESS);
  aclrtDestroyStream(stream);
}

TEST_F(MultiThreadExecutorE2ESystemTest, fail_then_success_by_kernel) {
  RunGraphFailThenSuccess(TaskProducerType::KERNEL);
}

TEST_F(MultiThreadExecutorE2ESystemTest, parallel_schedule_split_by_op) {
  TaskParallelScheduleBy(TaskProducerType::OP);
}

TEST_F(MultiThreadExecutorE2ESystemTest, parallel_schedule_split_by_kernel) {
  TaskParallelScheduleBy(TaskProducerType::KERNEL);
}

TEST_F(MultiThreadExecutorE2ESystemTest, parallel_schedule_split_by_chain) {
  TaskParallelScheduleBy(TaskProducerType::CHAIN);
}

TEST_F(MultiThreadExecutorE2ESystemTest, parallel_schedule_split_by_single) {
  TaskParallelScheduleBy(TaskProducerType::SINGLE);
}

TEST_F(MultiThreadExecutorE2ESystemTest, parallel_schedule_split_by_kernel_in_if_graph_true_branch) {
  auto pred_holder = TensorFaker().Placement(kOnHost).DataType(ge::DT_INT32).Value<int32_t>({1}).Build();
  RunIfGraph(pred_holder, true, TaskProducerType::KERNEL);
}

TEST_F(MultiThreadExecutorE2ESystemTest, parallel_schedule_split_by_kernel_in_if_graph_false_branch) {
  auto pred_holder = TensorFaker().Placement(kOnHost).DataType(ge::DT_INT32).Value<int32_t>({0}).Build();
  RunIfGraph(pred_holder, false, TaskProducerType::KERNEL);
}

TEST_F(MultiThreadExecutorE2ESystemTest, parallel_schedule_split_by_chain_in_if_graph_true_branch) {
  auto pred_holder = TensorFaker().Placement(kOnHost).DataType(ge::DT_INT32).Value<int32_t>({1}).Build();
  RunIfGraph(pred_holder, true, TaskProducerType::CHAIN);
}

TEST_F(MultiThreadExecutorE2ESystemTest, parallel_schedule_split_by_chain_in_if_graph_false_branch) {
  auto pred_holder = TensorFaker().Placement(kOnHost).DataType(ge::DT_INT32).Value<int32_t>({0}).Build();
  RunIfGraph(pred_holder, false, TaskProducerType::CHAIN);
}

TEST_F(MultiThreadExecutorE2ESystemTest, parallel_schedule_split_by_kernel_in_case_graph) {
  auto pred_holder = TensorFaker().Placement(kOnHost).DataType(ge::DT_INT32).Value<int32_t>({-1}).Build();
  RunCaseGraph(pred_holder, TaskProducerType::KERNEL);
}

TEST_F(MultiThreadExecutorE2ESystemTest, parallel_schedule_split_by_chain_in_case_graph) {
  auto pred_holder = TensorFaker().Placement(kOnHost).DataType(ge::DT_INT32).Value<int32_t>({-1}).Build();
  RunCaseGraph(pred_holder, TaskProducerType::CHAIN);
}

TEST_F(MultiThreadExecutorE2ESystemTest, parallel_schedule_split_by_kernel_in_while_graph) {
  RunWhileGraph(TaskProducerType::KERNEL);
}

TEST_F(MultiThreadExecutorE2ESystemTest, parallel_schedule_split_by_chain_in_while_graph) {
  RunWhileGraph(TaskProducerType::CHAIN);
}

TEST_F(MultiThreadExecutorE2ESystemTest, schedule_by_kernel_and_profiling_success) {
  auto graph = ShareGraph::LstmpGraph();
  graph->TopologicalSorting();
  GeModelBuilder builder(graph);
  auto ge_root_model =
      builder
          .AddTaskDef("TransData",
                      AiCoreTaskDefFaker(TransDataStubName).AtomicStubNum(DynamicAtomicStubName).WithHandle())
          .AddTaskDef("DynamicRNNV3", AiCoreTaskDefFaker(DynamicRnnv3StubName))
          .BuildGeRootModel();

  bg::ValueHolder::PopGraphFrame();  // 不需要BgTest自带的Frame
  auto exe_graph = ModelConverter().ConvertGeModelToExecuteGraph(ge_root_model);
  ASSERT_NE(exe_graph, nullptr);
  ASSERT_EQ(3, exe_graph->GetDirectNodesSize());

  GertRuntimeStub runtime_stub;
  runtime_stub.GetKernelStub().StubTiling();

  auto option = MultiThreadExecutorOption(3U);
  auto model_executor = ModelV2Executor::Create(exe_graph, option, ge_root_model);
  ASSERT_NE(model_executor, nullptr);

  ge::diagnoseSwitch::EnableProfiling({ProfilingType::kTaskTime});

  size_t report_event_count = 0U;
  auto default_check_func = [&](uint32_t moduleId, uint32_t type, void *data, uint32_t len) -> int32_t {
    if (type == InfoType::kEvent) {
      ++report_event_count;
    }
    return 0;
  };
  ProfilingTestUtil::Instance().SetProfFunc(default_check_func);

  auto allocator = memory::CachingMemAllocator::GetAllocator();
  auto mem_block = allocator->Malloc(2048 * 2);
  memset_s(const_cast<void *>(mem_block->GetAddr()), mem_block->GetSize(), 0, 2048 * 2);

  auto multi_thread_ed = reinterpret_cast<const MultiThreadExecutionData *>(
      model_executor->GetExeGraphExecutor(SubExeGraphType::kMainExeGraph)->GetExecutionData());
  multi_thread_ed->scheduler->DumpBrief();
  ASSERT_EQ(model_executor->Load(), ge::GRAPH_SUCCESS);
  auto outputs = FakeTensors({2048}, 3, const_cast<void *>(mem_block->GetAddr()));

  auto i0 = FakeValue<Tensor>(Tensor{{{2048}, {2048}},
                                     {ge::FORMAT_ND, ge::FORMAT_ND, {}},
                                     kOnHost,
                                     ge::DT_FLOAT16,
                                     const_cast<void *>(mem_block->GetAddr())});
  auto i1 = FakeValue<Tensor>(Tensor{{{2048}, {2048}},
                                     {ge::FORMAT_ND, ge::FORMAT_ND, {}},
                                     kOnHost,
                                     ge::DT_FLOAT16,
                                     const_cast<void *>(mem_block->GetAddr())});
  auto i2 = FakeValue<Tensor>(Tensor{{{2048}, {2048}},
                                     {ge::FORMAT_ND, ge::FORMAT_ND, {}},
                                     kOnHost,
                                     ge::DT_FLOAT16,
                                     const_cast<void *>(mem_block->GetAddr())});

  rtStream_t stream;
  ASSERT_EQ(aclrtCreateStreamWithConfig(&stream, static_cast<uint32_t>(RT_STREAM_PRIORITY_DEFAULT), 0), RT_ERROR_NONE);
  auto i3 = FakeValue<uint64_t>(reinterpret_cast<uint64_t>(stream));
  ASSERT_EQ(model_executor->Execute({i3.value},
                                    std::vector<Tensor *>({i0.holder.get(), i1.holder.get(), i2.holder.get()}).data(),
                                    3, reinterpret_cast<Tensor **>(outputs.GetAddrList()), outputs.size()),
            ge::GRAPH_SUCCESS);
  ASSERT_EQ(model_executor->UnLoad(), ge::GRAPH_SUCCESS);
  ASSERT_TRUE(runtime_stub.CheckLaunchWhenStubTiling());

  EXPECT_EQ(report_event_count, 6);
  multi_thread_ed->scheduler->Dump();
  aclrtDestroyStream(stream);
  mem_block->Free();
  ProfilingTestUtil::Instance().Clear();
  ge::diagnoseSwitch::MutableProfiling().SetEnableFlag(0);
}

namespace {
struct AdjacentGroupPair {
  ge::FastNode *lower_priority_node{nullptr};
  ge::FastNode *higher_priority_node{nullptr};
};

struct LstmpReverseEdgeModel {
  ge::GeRootModelPtr root_model;
  ge::ExecuteGraphPtr exe_graph;
  std::string name_a;
  std::string name_b;
  std::string type_a;
  std::string type_b;
};

bool IsNodeReachable(ge::FastNode *src, ge::FastNode *dst) {
  std::set<ge::FastNode *> seen;
  std::queue<ge::FastNode *> pending;
  pending.push(src);
  while (!pending.empty()) {
    auto *curr = pending.front();
    pending.pop();
    for (auto *out : curr->GetAllOutNodes()) {
      if (out == dst) {
        return true;
      }
      if (seen.insert(out).second) {
        pending.push(out);
      }
    }
  }
  return false;
}

// 按priority分组(与EnsureNodeExeInOrderInSubgraph一致), 选取相邻单节点组(A低值组, B高值组)且A到B无可达路径的一对
AdjacentGroupPair FindAdjacentGroupPairWithoutPath(const ge::ExecuteGraph *main_graph) {
  std::map<int64_t, std::vector<ge::FastNode *>> priority_groups;
  for (auto *node : main_graph->GetDirectNode()) {
    if (!IsLaunchOrHasSubGraphNode(node)) {
      continue;
    }
    int64_t priority = -1;
    (void)ge::AttrUtils::GetInt(node->GetOpDescBarePtr(), "priority", priority);
    priority_groups[priority].push_back(node);
  }
  AdjacentGroupPair group_pair;
  for (auto it = priority_groups.begin(); it != priority_groups.end() && group_pair.lower_priority_node == nullptr;
       ++it) {
    auto next_it = std::next(it);
    if (next_it == priority_groups.end() || it->second.size() != 1UL || next_it->second.size() != 1UL) {
      continue;
    }
    if (IsNodeReachable(it->second[0], next_it->second[0])) {
      continue;
    }
    group_pair.lower_priority_node = it->second[0];
    group_pair.higher_priority_node = next_it->second[0];
  }
  return group_pair;
}

// 执行节点命名规则: <kernel类型>_<计算节点名>_<id>, 提取计算节点名用于ess断言, 不匹配时返回空串
std::string ComputeNodeNameOfLaunch(const ge::FastNode *node) {
  const std::string name = node->GetName();
  const std::string prefix = std::string(node->GetType()) + "_";
  const auto last_sep = name.find_last_of('_');
  if (name.compare(0, prefix.size(), prefix) != 0 || last_sep <= prefix.size()) {
    return "";
  }
  return name.substr(prefix.size(), last_sep - prefix.size());
}

// 构造LSTMP网络并lowering, 在相邻优先级组间注入反向真实控制边B->A(模拟atomic算子order-holder边与优先级序反向)
LstmpReverseEdgeModel PrepareLstmpWithReverseCtrlEdge() {
  LstmpReverseEdgeModel model;
  auto graph = ShareGraph::LstmpGraph();
  graph->TopologicalSorting();
  GeModelBuilder builder(graph);
  model.root_model =
      builder
          .AddTaskDef("TransData",
                      AiCoreTaskDefFaker(TransDataStubName).AtomicStubNum(DynamicAtomicStubName).WithHandle())
          .AddTaskDef("DynamicRNNV3", AiCoreTaskDefFaker(DynamicRnnv3StubName))
          .BuildGeRootModel();
  bg::ValueHolder::PopGraphFrame();  // 不需要BgTest自带的Frame
  model.exe_graph = ModelConverter().ConvertGeModelToExecuteGraph(model.root_model);
  if (model.exe_graph == nullptr) {
    return model;
  }
  auto main_node = ge::ExecuteGraphUtils::FindFirstNodeMatchType(model.exe_graph.get(), "Main");
  auto main_graph = (main_node == nullptr) ? nullptr : ge::FastNodeUtils::GetSubgraphFromNode(main_node, 0);
  if (main_graph == nullptr) {
    return model;
  }
  auto group_pair = FindAdjacentGroupPairWithoutPath(main_graph);
  if (group_pair.lower_priority_node == nullptr || group_pair.higher_priority_node == nullptr) {
    return model;
  }
  if (!IsNodeReachable(group_pair.higher_priority_node, group_pair.lower_priority_node) &&
      main_graph->AddEdge(group_pair.higher_priority_node, ge::kControlEdgeIndex, group_pair.lower_priority_node,
                          ge::kControlEdgeIndex) == nullptr) {
    return model;
  }
  model.name_a = ComputeNodeNameOfLaunch(group_pair.lower_priority_node);
  model.name_b = ComputeNodeNameOfLaunch(group_pair.higher_priority_node);
  model.type_a = group_pair.lower_priority_node->GetType();
  model.type_b = group_pair.higher_priority_node->GetType();
  return model;
}
}  // namespace

/**
 * 用例描述： 相邻优先级组间存在反向真实控制边(高优先级值launch指向低优先级值launch, 模拟atomic算子
 *       order-holder边场景)时, 多线程执行器构建期的保序补边跳过该组对, 不与真实边构成环, 全图完整执行
 *
 * 预置条件：
 * 1. Fake LSTMP网络中算子的整套实现(TransData带AtomicClean原子kernel、DynamicRNNV3)
 * 2. fixture 已设置 MAX_RUNTIME_CORE_NUMBER=3, 走多线程执行器
 *
 * 测试步骤：
 * 1. 构造LSTMP网络, lowering成执行图, 在相邻优先级组间注入反向真实控制边B->A
 * 2. 创建多线程执行器, 挂接ess, Load后执行两轮
 * 3. 校验A/B每轮的执行次数与执行顺序
 *
 * 预期结果：
 * 1. Load/Execute均成功
 * 2. A、B每轮各执行1次, 且B先于A执行(反向真实边是有效的topo序约束, 运行期由StopGenWatcher正确处理)
 * 3. 保序补边因B->A反向直连边存在而跳过A->B补边; 若不跳过(修复回退), A->B与B->A构成2节点环,
 *    A/B及下游静默不执行, 执行次数为0, 本用例失败
 */
TEST_F(MultiThreadExecutorE2ESystemTest, reverse_ctrl_edge_skip_additional_order_edge) {
  auto model = PrepareLstmpWithReverseCtrlEdge();
  ASSERT_NE(model.exe_graph, nullptr);
  ASSERT_FALSE(model.name_a.empty());
  ASSERT_FALSE(model.name_b.empty());

  GertRuntimeStub runtime_stub;
  runtime_stub.GetKernelStub().StubTiling();
  auto option = MultiThreadExecutorOption(kThreadNum);
  auto model_executor = ModelV2Executor::Create(model.exe_graph, option, model.root_model);
  ASSERT_NE(model_executor, nullptr);
  auto ess = StartExecutorStatistician(model_executor);

  auto allocator = memory::CachingMemAllocator::GetAllocator();
  auto mem_block = allocator->Malloc(2048 * 2);
  memset_s(const_cast<void *>(mem_block->GetAddr()), mem_block->GetSize(), 0, 2048 * 2);
  ASSERT_EQ(model_executor->Load(), ge::GRAPH_SUCCESS);
  auto outputs = FakeTensors({2048}, 3, const_cast<void *>(mem_block->GetAddr()));
  auto inputs =
      FakeTensors({2048}, 3, const_cast<void *>(mem_block->GetAddr()), kOnHost, ge::FORMAT_ND, ge::DT_FLOAT16);
  rtStream_t stream;
  ASSERT_EQ(aclrtCreateStreamWithConfig(&stream, static_cast<uint32_t>(RT_STREAM_PRIORITY_DEFAULT), 0), RT_ERROR_NONE);
  auto i3 = FakeValue<uint64_t>(reinterpret_cast<uint64_t>(stream));

  for (size_t round = 0; round < 2; ++round) {
    ess->Clear();
    ASSERT_EQ(model_executor->Execute({i3.value}, inputs.GetTensorList(), inputs.size(),
                                      reinterpret_cast<Tensor **>(outputs.GetAddrList()), outputs.size()),
              ge::GRAPH_SUCCESS);
    ASSERT_EQ(ess->GetExecuteCountByNodeNameAndKernelType(model.name_a, model.type_a), 1);
    ASSERT_EQ(ess->GetExecuteCountByNodeNameAndKernelType(model.name_b, model.type_b), 1);
    ASSERT_LT(ess->GetExecuteIndexByNodeNameAndKernelType(model.name_b, model.type_b),
              ess->GetExecuteIndexByNodeNameAndKernelType(model.name_a, model.type_a));
  }

  ASSERT_EQ(model_executor->UnLoad(), ge::GRAPH_SUCCESS);
  ASSERT_TRUE(runtime_stub.CheckLaunchWhenStubTiling());
  aclrtDestroyStream(stream);
  mem_block->Free();
}

}  // namespace gert
