/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "engine/aicpu/converter/aicpu_node_converter.h"

#include <gtest/gtest.h>
#include <map>

#include "engine/gelocal/inputs_converter.h"
#include "graph/utils/graph_utils.h"
#include "graph_builder/value_holder_generator.h"
#include "common/share_graph.h"
#include "faker/global_data_faker.h"
#include "common/bg_test.h"
#include "faker/aicpu_taskdef_faker.h"
#include "framework/common/ge_types.h"
#include "mmpa/mmpa_api.h"
#include "stub/hostcpu_mmpa_stub.h"
#include "stub/kernel_stub.h"
#include "runtime/fast_v2/common/const_data_helper.h"
#include "graph/utils/node_utils.h"
#include "engine/aicpu/kernel/aicpu_resource_manager.h"

#include "graph/utils/execute_graph_utils.h"
#include "graph/debug/ge_attr_define.h"
#include "tests/ge/ut/ge/graph/load/om2_package_test_utils.h"
namespace gert {
namespace {
using namespace ge;
void InitHostCpuUtEnv() {
  char_t opp_path[MMPA_MAX_PATH] = "./";
  mmSetEnv("ASCEND_OPP_PATH", &opp_path[0U], MMPA_MAX_PATH);
  ge::MmpaStub::GetInstance().SetImpl(std::make_shared<HostcpuMockMmpa>());
  ASSERT_EQ(AicpuResourceManager::GetInstance().LoadConstantFoldingLib(), GRAPH_SUCCESS);
}

void MakeMiddleOptionalInput(const ge::NodePtr &node, const ge::GeTensorDesc &input_desc,
                             const std::string &optional_name, const std::string &last_required_name,
                             const std::string &first_required_name = "x") {
  ASSERT_NE(node, nullptr);
  ASSERT_EQ(node->GetOpDesc()->AddInputDesc(optional_name, input_desc), GRAPH_SUCCESS);
  node->GetOpDesc()->MutableAllInputName() = {{first_required_name, 0}, {last_required_name, 1}, {optional_name, 2}};
  node->GetOpDesc()->AppendIrInput(first_required_name, ge::kIrInputRequired);
  node->GetOpDesc()->AppendIrInput(optional_name, ge::kIrInputOptional);
  node->GetOpDesc()->AppendIrInput(last_required_name, ge::kIrInputRequired);
  ASSERT_TRUE(ge::AttrUtils::SetBool(node->GetOpDesc(), "optional_input_placeholder", true));
}

void PopExecuteGraph(const std::vector<bg::DevMemValueHolderPtr> &out_addrs,
                     const std::vector<bg::ValueHolderPtr> &order_holders, ge::ExecuteGraphPtr &execute_graph) {
  auto graph_frame = bg::ValueHolder::PopGraphFrame(ConvertDevMemValueHoldersToValueHolders(out_addrs), order_holders);
  ASSERT_NE(graph_frame, nullptr);
  execute_graph = graph_frame->GetExecuteGraph();
  ASSERT_NE(execute_graph, nullptr);
}

graphStatus Om2HostMetadataProbe(KernelContext *) {
  return GRAPH_SUCCESS;
}
class Om2HostMetadataUT : public bg::BgTestAutoCreate3StageFrame, public testing::WithParamInterface<bool> {
 protected:
  void TearDown() override {
    kernel_stub_.Clear();
    ge::MmpaStub::GetInstance().Reset();
    bg::BgTestAutoCreate3StageFrame::TearDown();
  }
  KernelStub kernel_stub_;
};
TEST_P(Om2HostMetadataUT, OriginalTypeIsWrittenOnlyForOm2WithoutChangingOptionalInputs) {
  ge::om2_test::ScopedEnvVar opp("ASCEND_OPP_PATH", "./");
  ASSERT_NO_FATAL_FAILURE(InitHostCpuUtEnv());

  auto graph = ShareGraph::BuildSingleNodeGraph();
  ASSERT_NE(graph, nullptr);

  auto data1 = graph->FindNode("data1");
  auto data2 = graph->FindNode("data2");
  auto add1 = graph->FindNode("add1");
  ASSERT_NE(data1, nullptr);
  ASSERT_NE(data2, nullptr);
  ASSERT_NE(add1, nullptr);

  add1->GetOpDesc()->SetOpKernelLibName(ge::kEngineNameHostCpu.c_str());
  MakeMiddleOptionalInput(add1, data1->GetOpDesc()->GetOutputDesc(0), "y", "z");
  bool optional_placeholder = false;
  ASSERT_TRUE(ge::AttrUtils::GetBool(add1->GetOpDesc(), "optional_input_placeholder", optional_placeholder));
  ASSERT_TRUE(optional_placeholder);
  graph->TopologicalSorting();

  AiCpuCCTaskDefFaker aicpu_task_def_faker;
  auto root_model = GeModelBuilder(graph).BuildGeRootModel();
  auto global_data = GlobalDataFaker(root_model).AddTaskDef("Add", aicpu_task_def_faker).Build();
  global_data.SetForOm2(GetParam());
  bg::LowerConstDataNode(global_data);
  LowerInput data_input = {{}, {}, &global_data};

  auto data1_ret = LoweringDataNode(data1, data_input);
  auto data2_ret = LoweringDataNode(data2, data_input);
  ASSERT_TRUE(data1_ret.result.IsSuccess());
  ASSERT_TRUE(data2_ret.result.IsSuccess());

  LowerInput add_input = {{data1_ret.out_shapes[0], data2_ret.out_shapes[0]},
                          {data1_ret.out_addrs[0], data2_ret.out_addrs[0]},
                          &global_data};
  ASSERT_TRUE(kernel_stub_.SetUp("AddHostKernel", Om2HostMetadataProbe));
  auto add_ret = LoweringAiCpuNode(add1, add_input);
  ASSERT_TRUE(add_ret.result.IsSuccess());

  ge::ExecuteGraphPtr execute_graph = nullptr;
  ASSERT_NO_FATAL_FAILURE(PopExecuteGraph(add_ret.out_addrs, add_ret.order_holders, execute_graph));
  auto host_exec_node = ge::ExecuteGraphUtils::FindFirstNodeMatchType(execute_graph.get(), "AicpuHostExecFunc");
  ASSERT_NE(host_exec_node, nullptr);
  ASSERT_NE(host_exec_node->GetOpDescPtr(), nullptr);
  ASSERT_EQ(host_exec_node->GetOpDescPtr()->GetAllInputsSize(), 7U);
  std::string original_type;
  EXPECT_EQ(AttrUtils::GetStr(host_exec_node->GetOpDescPtr(), ATTR_NAME_FRAMEWORK_ORIGINAL_TYPE, original_type),
            GetParam());
  EXPECT_EQ(original_type, GetParam() ? "Add" : "");
}

INSTANTIATE_TEST_SUITE_P(Rt2AndOm2, Om2HostMetadataUT, testing::Bool());
}  // namespace
}  // namespace gert
