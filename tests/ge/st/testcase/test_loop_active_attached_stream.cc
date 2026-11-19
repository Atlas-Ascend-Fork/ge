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
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "common/share_graph.h"
#include "faker/space_registry_faker.h"
#include "ge/ge_api.h"
#include "ge/ge_api_types.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/graph_utils_ex.h"
#include "graph/custom_op_factory.h"
#include "framework/common/framework_types_internal.h"
#include "mmpa/mmpa_api.h"
#include "utils/mock_ops_kernel_builder.h"
#include "stub/gert_runtime_stub.h"
#include "ge_graph_dsl/graph_dsl.h"
#include "exe_graph/runtime/storage_shape.h"
#include "init_ge.h"
#include "common/global_variables/diagnose_switch.h"

using namespace std;

namespace ge {
using namespace gert;
namespace {
constexpr const char *kLoopAuxOpType = "LoopAuxRequestOp";

// Eager 自定义算子：Execute（模型加载/下沉阶段执行一次）中按 key 申请框架托管辅流，
// 记录句柄供用例校验循环激活 task 是否对其追加了激活
class LoopAuxRequestCustomOp final : public EagerExecuteOp {
 public:
  graphStatus Execute(gert::EagerOpExecutionContext *ctx) override {
    requested_stream_ = ctx->RequestAttachedStream("loop_aux", 0U);
    GE_ASSERT_NOTNULL(requested_stream_);
    auto output_tensor = ctx->MallocOutputTensor(0, gert::StorageShape({2, 2, 2}, {2, 2, 2}),
                                                 gert::StorageFormat(FORMAT_ND, FORMAT_ND, ExpandDimsType()), DT_FLOAT);
    GE_ASSERT_NOTNULL(output_tensor);
    return GRAPH_SUCCESS;
  }

  static rtStream_t requested_stream_;
};
rtStream_t LoopAuxRequestCustomOp::requested_stream_ = nullptr;

Status GenerateTaskForCustomOp(const Node &node, RunContext &run_context, std::vector<domi::TaskDef> &tasks) {
  (void)run_context;
  domi::TaskDef task_def = {};
  task_def.set_stream_id(static_cast<uint32_t>(node.GetOpDesc()->GetStreamId()));
  task_def.set_type(static_cast<uint32_t>(ModelTaskType::MODEL_TASK_CUSTOM_KERNEL));
  domi::KernelDef *kernel_def = task_def.mutable_kernel();
  kernel_def->mutable_context()->set_op_index(node.GetOpDesc()->GetId());
  tasks.push_back(task_def);
  return SUCCESS;
}

// 模拟 RTS 引擎为流控节点生成 task（对齐 stream_active_op.cc 的行为），
// 使编译产物中存在 StreamActiveTaskInfo，覆盖循环激活 task 的 Distribute 路径
Status GenerateTaskForRtsCtrl(const Node &node, RunContext &run_context, std::vector<domi::TaskDef> &tasks) {
  (void)run_context;
  const auto op_desc = node.GetOpDesc();
  if (node.GetType() == STREAMACTIVE) {
    std::vector<int64_t> active_stream_list;
    if (!AttrUtils::GetListInt(op_desc, ATTR_NAME_ACTIVE_STREAM_LIST, active_stream_list)) {
      return SUCCESS;
    }
    for (const auto &stream_id : active_stream_list) {
      domi::TaskDef task_def = {};
      task_def.set_type(static_cast<uint32_t>(ModelTaskType::MODEL_TASK_STREAM_ACTIVE));
      task_def.set_stream_id(static_cast<uint32_t>(op_desc->GetStreamId()));
      domi::StreamActiveDef *stream_active_def = task_def.mutable_stream_active();
      stream_active_def->set_op_index(static_cast<uint32_t>(op_desc->GetId()));
      stream_active_def->set_active_stream_id(static_cast<uint32_t>(stream_id));
      tasks.push_back(task_def);
    }
  } else if (node.GetType() == STREAMSWITCH) {
    domi::TaskDef task_def = {};
    task_def.set_type(static_cast<uint32_t>(ModelTaskType::MODEL_TASK_STREAM_SWITCH));
    task_def.set_stream_id(static_cast<uint32_t>(op_desc->GetStreamId()));
    domi::StreamSwitchDef *stream_switch_def = task_def.mutable_stream_switch();
    stream_switch_def->set_op_index(static_cast<uint32_t>(op_desc->GetId()));
    tasks.push_back(task_def);
  }
  return SUCCESS;
}

GeTensorPtr GenerateLoopVarTensor(int64_t data_value) {
  GeTensorDesc data_tensor_desc(GeShape(std::vector<int64_t>({})), FORMAT_ND, DT_INT64);
  return std::make_shared<GeTensor>(data_tensor_desc, reinterpret_cast<uint8_t *>(&data_value), sizeof(int64_t));
}
// 循环主体图：复用 ShareGraph 的 customop 已知 shape 图，仅把节点 type 改为唯一名
// （避免与其他用例注册到全局 CustomOpFactory 的 "CustomOp" creator 冲突导致 double free），
// NetOutput 标记 TRUE_BRANCH_STREAM=0 触发 FP/BP 大循环（FlowCtrlPass）
Graph BuildLoopGraph() {
  auto compute_graph = ShareGraph::BuildOnlyCustomOpKnowShapeGraph();
  auto custom_node = compute_graph->FindNode("custom_op");
  custom_node->GetOpDesc()->SetType(kLoopAuxOpType);
  auto net_output = compute_graph->FindNode("NetOutput");
  AttrUtils::SetInt(net_output->GetOpDesc(), ATTR_NAME_TRUE_BRANCH_STREAM, TRUE_STREAM_ID);
  auto graph = GraphUtilsEx::CreateGraphFromComputeGraph(compute_graph);
  graph.SetNeedIteration(true);
  return graph;
}

// FlowCtrl 所需的 5 个循环变量的初始化图（对齐 test_iterator_flow_ctrl 的构造方式）
Graph BuildLoopVarInitGraph(int64_t loop_num_per_iter = 10) {
  const std::vector<int64_t> shape = {};
  auto data_tensor = GenerateLoopVarTensor(1);
  auto loop_num_data_tensor = GenerateLoopVarTensor(loop_num_per_iter);
  DEF_GRAPH(flow_ctrl_var_init) {
    uint32_t output_port_id_idx = 0;
    const auto var_cfg = [&](const std::string &name) {
      return OP_CFG(VARIABLE).TensorDesc(FORMAT_ND, DT_INT64, shape).InCnt(1).OutCnt(1).Build(name);
    };
    const auto const_cfg = [&](const std::string &name, GeTensorPtr &t) {
      return OP_CFG(CONSTANT).TensorDesc(FORMAT_ND, DT_INT64, shape).Weight(t).InCnt(1).OutCnt(1).Build(name);
    };
    const auto assign_cfg = [&](const std::string &name) {
      return OP_CFG(ASSIGN).TensorDesc(FORMAT_ND, DT_INT64, shape).InCnt(2).OutCnt(1).Build(name);
    };

    CHAIN(NODE(var_cfg(NODE_NAME_GLOBAL_STEP))->NODE(assign_cfg("global_step_assign")));
    CHAIN(NODE(const_cfg("global_step_const", data_tensor))->EDGE(0, 1)->NODE("global_step_assign"));
    CHAIN(NODE(assign_cfg("global_step_assign"))->EDGE(0, output_port_id_idx)->NODE("NetOutput1", NETOUTPUT));
    ++output_port_id_idx;

    CHAIN(NODE(var_cfg(NODE_NAME_FLOWCTRL_LOOP_PER_ITER))->NODE(assign_cfg("loop_per_iter_assign")));
    CHAIN(NODE(const_cfg("loop_per_iter_const", loop_num_data_tensor))->EDGE(0, 1)->NODE("loop_per_iter_assign"));
    CHAIN(NODE(assign_cfg("loop_per_iter_assign"))->EDGE(0, output_port_id_idx)->NODE("NetOutput1"));
    ++output_port_id_idx;

    CHAIN(NODE(var_cfg(NODE_NAME_FLOWCTRL_LOOP_COND))->NODE(assign_cfg("loop_cond_assign")));
    CHAIN(NODE(const_cfg("loop_cond_const", data_tensor))->EDGE(0, 1)->NODE("loop_cond_assign"));
    CHAIN(NODE(assign_cfg("loop_cond_assign"))->EDGE(0, output_port_id_idx)->NODE("NetOutput1"));
    ++output_port_id_idx;

    CHAIN(NODE(var_cfg(NODE_NAME_FLOWCTRL_LOOP_INCREMENT))->NODE(assign_cfg("loop_inc_assign")));
    CHAIN(NODE(const_cfg("loop_inc_const", data_tensor))->EDGE(0, 1)->NODE("loop_inc_assign"));
    CHAIN(NODE(assign_cfg("loop_inc_assign"))->EDGE(0, output_port_id_idx)->NODE("NetOutput1"));
    ++output_port_id_idx;

    CHAIN(NODE(var_cfg(NODE_NAME_FLOWCTRL_LOOP_RESETVALUE))->NODE(assign_cfg("loop_reset_value_assign")));
    CHAIN(NODE(const_cfg("loop_reset_value_const", data_tensor))->EDGE(0, 1)->NODE("loop_reset_value_assign"));
    CHAIN(NODE(assign_cfg("loop_reset_value_assign"))->EDGE(0, output_port_id_idx)->NODE("NetOutput1"));
  };
  return ToGeGraph(flow_ctrl_var_init);
}

constexpr uint32_t kVarGraphId = 88;
constexpr uint32_t kLoopGraphId = 99;

// 注册 kLoopAuxOpType 的 shape/data_type/range 推导实现（编译 Eager 算子图的前置条件）
void RegisterLoopAuxOpImpl() {
  const auto infer_shape_func = [](gert::InferShapeContext *context) -> graphStatus {
    const auto input_shape = context->GetInputShape(0U);
    auto output = context->GetOutputShape(0);
    for (size_t dim = 0UL; dim < input_shape->GetDimNum(); dim++) {
      output->AppendDim(input_shape->GetDim(dim));
    }
    output->SetDimNum(input_shape->GetDimNum());
    return GRAPH_SUCCESS;
  };
  const auto infer_data_type_func = [](gert::InferDataTypeContext *context) -> graphStatus {
    const auto date_type = context->GetInputDataType(0U);
    EXPECT_EQ(context->SetOutputDataType(0, date_type), SUCCESS);
    return GRAPH_SUCCESS;
  };
  const auto infer_shape_range_func = [](gert::InferShapeRangeContext *context) -> graphStatus {
    auto input_shape_range = context->GetInputShapeRange(0U);
    auto output_shape_range = context->GetOutputShapeRange(0U);
    output_shape_range->SetMin(const_cast<gert::Shape *>(input_shape_range->GetMin()));
    output_shape_range->SetMax(const_cast<gert::Shape *>(input_shape_range->GetMax()));
    return GRAPH_SUCCESS;
  };
  gert::SpaceRegistryFaker::CreateDefaultSpaceRegistryImpl2(true);
  auto space_registry = gert::DefaultOpImplSpaceRegistryV2::GetInstance().GetSpaceRegistry();
  ASSERT_NE(space_registry, nullptr);
  auto op_impl_func = space_registry->CreateOrGetOpImpl(kLoopAuxOpType);
  op_impl_func->infer_shape = infer_shape_func;
  op_impl_func->infer_datatype = infer_data_type_func;
  op_impl_func->infer_shape_range = infer_shape_range_func;
  op_impl_func->output_shape_depend_compute = 1UL;
}

std::map<AscendString, AscendString> MakeSessionOptions() {
  std::map<AscendString, AscendString> options;
  options.emplace(ge::OPTION_CONST_LIFECYCLE, "graph");
  options.emplace(ge::OPTION_GRAPH_RUN_MODE, "1");
  return options;
}

// var 图先建，创建 FlowCtrl 依赖的循环变量
void BuildFlowCtrlVars(Session &session, const std::map<AscendString, AscendString> &options) {
  std::vector<InputTensorInfo> var_inputs;
  ASSERT_EQ(session.AddGraph(kVarGraphId, BuildLoopVarInitGraph(), options), SUCCESS);
  ASSERT_EQ(session.BuildGraph(kVarGraphId, var_inputs), SUCCESS);
}

// 循环图的常量/特征内存需在执行期间存活，由调用方持有返回的缓冲区
struct LoopGraphMemory {
  std::vector<uint8_t> weight_mem;
  std::vector<uint8_t> feature_mem;
};

void CompileLoopGraph(Session &session, const std::map<AscendString, AscendString> &options, LoopGraphMemory &memory) {
  ASSERT_EQ(session.AddGraph(kLoopGraphId, BuildLoopGraph(), options), SUCCESS);
  ASSERT_EQ(session.CompileGraph(kLoopGraphId), SUCCESS);

  const CompiledGraphSummaryPtr summary = session.GetCompiledGraphSummary(kLoopGraphId);
  ASSERT_NE(summary, nullptr);
  size_t weight_size = 0UL;
  size_t feature_size = 0UL;
  EXPECT_EQ(SUCCESS, summary->GetConstMemorySize(weight_size));
  EXPECT_EQ(SUCCESS, summary->GetFeatureMemorySize(feature_size));
  memory.weight_mem.assign(weight_size, 0);
  memory.feature_mem.assign(feature_size, 0);
  EXPECT_EQ(SUCCESS, session.SetGraphConstMemoryBase(kLoopGraphId, memory.weight_mem.data(), weight_size));
  EXPECT_EQ(SUCCESS, session.UpdateGraphFeatureMemoryBase(kLoopGraphId, memory.feature_mem.data(), feature_size));
}

void BuildLoopIoTensors(std::vector<ge::Tensor> &inputs, std::vector<ge::Tensor> &outputs) {
  for (size_t i = 0UL; i < 3UL; i++) {
    std::vector<float32_t> input_data(2 * 2 * 2, 0);
    TensorDesc desc(Shape({2, 2, 2}));
    ge::Tensor input_tensor{desc};
    input_tensor.SetData(reinterpret_cast<uint8_t *>(input_data.data()), input_data.size() * sizeof(float32_t));
    inputs.emplace_back(input_tensor);
  }
  TensorDesc output_desc(Shape({2, 2, 2}));
  ge::Tensor output_tensor{output_desc};
  std::vector<uint8_t> output_data(2 * 2 * 2 * sizeof(float32_t), 0);
  output_tensor.SetData(output_data.data(), output_data.size());
  outputs.emplace_back(output_tensor);
}

// 追加激活断言：目标流为辅流，载体为循环激活 task 所在的模型流，二者不同
void ExpectAppendedActivation(const gert::GertRuntimeStub &runtime_stub, rtStream_t attached_stream) {
  const auto &active_records = runtime_stub.GetAclRuntimeStub().GetActiveStreamRecords();
  ASSERT_FALSE(active_records.empty());
  bool appended_found = false;
  for (const auto &record : active_records) {
    if (record.active_stream == attached_stream) {
      appended_found = true;
      EXPECT_NE(record.carrier_stream, nullptr);
      EXPECT_NE(record.carrier_stream, attached_stream);
    }
  }
  EXPECT_TRUE(appended_found);
}
}  // namespace

class LoopActiveAttachedStreamStTest : public testing::Test {
 protected:
  void SetUp() override {
    char runtime2_env[MMPA_MAX_PATH] = {'0'};
    mmSetEnv("ENABLE_RUNTIME_V2", &(runtime2_env[0U]), static_cast<uint32_t>(MMPA_MAX_PATH));
    // 前序用例可能已将 GELib 置于 finalized 状态，此处重建初始化，保证 BuildGraph/CompileGraph 可用
    ReInitGe();
    LoopAuxRequestCustomOp::requested_stream_ = nullptr;
    // 既有用例（如 CustomOpRefreshTest）TearDown 会注销全局注册的 AiCoreLib，
    // 此处重新注册一个与默认 FakeOpsKernelBuilder 行为一致的 no-op builder 自愈，
    // 否则 var 图编译阶段 CalcOpRunningParam 找不到 libName 直接失败
    MockForGenerateTask("AiCoreLib", [](const Node &, RunContext &, std::vector<domi::TaskDef> &) { return SUCCESS; });
  }
  void TearDown() override {
    OpsKernelBuilderRegistry::GetInstance().Unregister("RTSLib");
    TearDownForGenerateTask(kCustomOpKernelLibName);
    ReInitGe();
    char runtime2_env[MMPA_MAX_PATH] = {'1'};
    mmSetEnv("ENABLE_RUNTIME_V2", &(runtime2_env[0U]), static_cast<uint32_t>(MMPA_MAX_PATH));
    LoopAuxRequestCustomOp::requested_stream_ = nullptr;
  }
};

/**
 * 用例描述：训练模式循环图（IteratorCtrl）加载时，循环激活 task 追加激活 Eager 自定义算子
 * 在 Execute 中申请的框架托管辅流，保证模型重放（第二遍迭代起）辅流上任务每遍被激活
 * 预置条件：
 *   1. 注册 Eager 自定义算子（Execute 中按 key 申请辅流并记录句柄），op type 唯一避免全局 registry 污染
 *   2. mock DNN_VM_CUSTOM_OP_STORE 与 RTSLib 的任务生成，RTSLib 生成 StreamActive/StreamSwitch task
 *   3. var 初始化图提供 FlowCtrl 所需的循环变量
 * 测试步骤：
 *   1. 构造循环图：data0/1/2 -> customop -> NetOutput(TRUE_BRANCH_STREAM=0)，SetNeedIteration(true)
 *   2. Session 编译并执行（加载触发 Execute -> 申请辅流 -> 登记循环激活集合）
 *   3. 校验 aclrtActiveStream 调用记录
 * 预期结果：
 *   1. 编译、执行均成功
 *   2. 辅流申请成功（句柄非空）
 *   3. aclrtActiveStream 以该辅流为目标流被调用（追加激活存在），载体流非辅流自身
 */
TEST_F(LoopActiveAttachedStreamStTest, loop_active_task_activates_attached_stream_on_load) {
  // slow_test_limit = 3000
  // 端到端用例：GELib 重建 + var 图/循环图两次建图编译 + 模型加载执行，耗时超 1s 默认阈值
  MockForGenerateTask(kCustomOpKernelLibName, GenerateTaskForCustomOp);
  MockForGenerateTask("RTSLib", GenerateTaskForRtsCtrl);

  gert::GertRuntimeStub runtime_stub;
  CustomOpFactory::RegisterCustomOpCreator(
      kLoopAuxOpType, []() -> std::unique_ptr<BaseCustomOp> { return std::make_unique<LoopAuxRequestCustomOp>(); });
  RegisterLoopAuxOpImpl();

  const auto options = MakeSessionOptions();
  Session session(options);
  BuildFlowCtrlVars(session, options);

  LoopGraphMemory memory;
  CompileLoopGraph(session, options, memory);

  std::vector<ge::Tensor> inputs;
  std::vector<ge::Tensor> outputs;
  BuildLoopIoTensors(inputs, outputs);

  ge::diagnoseSwitch::DisableDumper();
  runtime_stub.Clear();
  // 首次执行触发模型下沉：Execute 申请辅流并登记，循环激活 task Distribute 时追加激活
  EXPECT_EQ(SUCCESS, session.RunGraphWithStreamAsync(kLoopGraphId, nullptr, inputs, outputs));

  ASSERT_NE(LoopAuxRequestCustomOp::requested_stream_, nullptr);
  ExpectAppendedActivation(runtime_stub, LoopAuxRequestCustomOp::requested_stream_);
  runtime_stub.Clear();
}
}  // namespace ge
