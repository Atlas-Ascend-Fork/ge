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
#include <vector>

#include "omg/omg_inner_types.h"

#include "macro_utils/dt_public_scope.h"
#include "graph/passes/feature/net_output_pass.h"
#include "graph/passes/standard_optimize/prune_pass.h"

#include "anchor.h"
#include "common/debug/log.h"
#include "common/debug/memory_dumper.h"
#include "common/framework_types_internal.h"
#include "common/ge_common/ge_types.h"
#include "framework/common/ge_inner_error_codes.h"
#include "graph/attr_value.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/op_desc_utils_ex.h"
#include "graph/passes/pass_manager.h"
#include "macro_utils/dt_public_unscope.h"
#include "graph_builder_utils.h"
#include "ge_graph_dsl/graph_dsl.h"

using namespace testing;
using namespace ge;
using namespace std;

class UtestGraphPassesPrunePass : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}
};

// case1:no net_out_put_node
TEST_F(UtestGraphPassesPrunePass, no_net_out_put_node) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("default");

  ge::OpDescPtr reverse_op = std::make_shared<ge::OpDesc>();
  ge::OpDescUtilsEx::SetType(reverse_op, REVERSE);
  reverse_op->SetName("Reverse");
  reverse_op->AddOutputDesc(ge::GeTensorDesc());
  ge::NodePtr reverse_node = graph->AddNode(reverse_op);

  ge::OpDescPtr floor_op = std::make_shared<ge::OpDesc>();
  ge::OpDescUtilsEx::SetType(floor_op, FLOOR);
  floor_op->SetName("Floor");
  floor_op->AddInputDesc(ge::GeTensorDesc());
  floor_op->AddOutputDesc(ge::GeTensorDesc());
  ge::NodePtr floor_node = graph->AddNode(floor_op);

  ge::GraphUtils::AddEdge(reverse_node->GetOutDataAnchor(0), floor_node->GetInDataAnchor(0));

  uint64_t size_ori = graph->GetDirectNode().size();
  PrunePass prune_pass;
  std::vector<std::pair<string, GraphPass *>> passes = {{"PrunePass", &prune_pass}};
  Status status = PassManager::Run(graph, passes);

  EXPECT_EQ(ge::SUCCESS, status);

  uint64_t size = graph->GetDirectNode().size();
  EXPECT_TRUE(size == size_ori);
}
// case2: one net path with one bypass branch
TEST_F(UtestGraphPassesPrunePass, has_net_out_put_node_with_only_one_path) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("default");

  ge::OpDescPtr reverse_op = std::make_shared<ge::OpDesc>();
  ge::OpDescUtilsEx::SetType(reverse_op, REVERSE);
  reverse_op->SetName("Reverse");
  reverse_op->AddOutputDesc(ge::GeTensorDesc());
  ge::NodePtr reverse_node = graph->AddNode(reverse_op);

  ge::OpDescPtr floor_op = std::make_shared<ge::OpDesc>();
  ge::OpDescUtilsEx::SetType(floor_op, FLOOR);
  floor_op->SetName("Floor");
  floor_op->AddInputDesc(ge::GeTensorDesc());
  floor_op->AddOutputDesc(ge::GeTensorDesc());
  ge::NodePtr floor_node = graph->AddNode(floor_op);

  ge::OpDescPtr net_output_op = std::make_shared<ge::OpDesc>(NODE_NAME_NET_OUTPUT, NETOUTPUT);
  net_output_op->AddInputDesc(ge::GeTensorDesc());
  net_output_op->AddOutputDesc(ge::GeTensorDesc());
  ge::AttrUtils::SetBool(net_output_op, "identity_add_netoutput", true);
  ge::NodePtr netoutput_node = graph->AddNode(net_output_op);

  ge::OpDescPtr reverse_op1 = std::make_shared<ge::OpDesc>();
  ge::OpDescUtilsEx::SetType(reverse_op, REVERSE);
  reverse_op->SetName("Reverse1");
  reverse_op->AddOutputDesc(ge::GeTensorDesc());
  ge::NodePtr reverse_node1 = graph->AddNode(reverse_op1);

  ge::GraphUtils::AddEdge(reverse_node->GetOutDataAnchor(0), floor_node->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(floor_node->GetOutDataAnchor(0), netoutput_node->GetInDataAnchor(0));

  uint64_t size_ori = graph->GetDirectNode().size();
  PrunePass prune_pass;
  std::vector<std::pair<string, GraphPass *>> passes = {{"PrunePass", &prune_pass}};
  Status status = PassManager::Run(graph, passes);

  uint64_t size = graph->GetDirectNode().size();
  int diff = size_ori - size;
  EXPECT_EQ(ge::SUCCESS, status);
  EXPECT_EQ(diff, 1);
}
// case3: one net path with one bypass branch
TEST_F(UtestGraphPassesPrunePass, has_net_out_put_node_with_one_valid_path_and_one_bypass_path) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("default");

  // valid path construct (reverse->floor->net_out)
  ge::OpDescPtr reverse_op = std::make_shared<ge::OpDesc>();
  ge::OpDescUtilsEx::SetType(reverse_op, REVERSE);
  reverse_op->SetName("Reverse");
  reverse_op->AddOutputDesc(ge::GeTensorDesc());
  reverse_op->AddOutputDesc(ge::GeTensorDesc());
  ge::NodePtr reverse_node = graph->AddNode(reverse_op);

  ge::OpDescPtr floor_op = std::make_shared<ge::OpDesc>();
  ge::OpDescUtilsEx::SetType(floor_op, FLOOR);
  floor_op->SetName("Floor");
  floor_op->AddInputDesc(ge::GeTensorDesc());
  floor_op->AddOutputDesc(ge::GeTensorDesc());
  ge::NodePtr floor_node = graph->AddNode(floor_op);

  ge::OpDescPtr net_output_op = std::make_shared<ge::OpDesc>(NODE_NAME_NET_OUTPUT, NETOUTPUT);
  net_output_op->AddInputDesc(ge::GeTensorDesc());
  net_output_op->AddOutputDesc(ge::GeTensorDesc());
  ge::AttrUtils::SetBool(net_output_op, "identity_add_netoutput", true);
  ge::NodePtr netoutput_node = graph->AddNode(net_output_op);

  ge::GraphUtils::AddEdge(reverse_node->GetOutDataAnchor(0), floor_node->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(floor_node->GetOutDataAnchor(0), netoutput_node->GetInDataAnchor(0));

  // invalid path construct (reverse->floor1->floor2)
  ge::OpDescPtr floor_op1 = std::make_shared<ge::OpDesc>();
  ge::OpDescUtilsEx::SetType(floor_op1, FLOOR);
  floor_op1->SetName("Floor1");
  floor_op1->AddInputDesc(ge::GeTensorDesc());
  floor_op1->AddOutputDesc(ge::GeTensorDesc());
  ge::NodePtr floor_node1 = graph->AddNode(floor_op1);

  ge::OpDescPtr floor_op2 = std::make_shared<ge::OpDesc>();
  ge::OpDescUtilsEx::SetType(floor_op2, FLOOR);
  floor_op2->SetName("Floor2");
  floor_op2->AddInputDesc(ge::GeTensorDesc());
  floor_op2->AddOutputDesc(ge::GeTensorDesc());
  ge::NodePtr floor_node2 = graph->AddNode(floor_op2);
  // isolated node
  ge::OpDescPtr floor_op3 = std::make_shared<ge::OpDesc>();
  ge::OpDescUtilsEx::SetType(floor_op3, FLOOR);
  floor_op3->SetName("Floor3");
  floor_op3->AddInputDesc(ge::GeTensorDesc());
  floor_op3->AddOutputDesc(ge::GeTensorDesc());
  ge::NodePtr floor_node3 = graph->AddNode(floor_op3);

  EXPECT_EQ(ge::GraphUtils::AddEdge(reverse_node->GetOutDataAnchor(1), floor_node1->GetInDataAnchor(0)),
            ge::GRAPH_SUCCESS);
  EXPECT_EQ(ge::GraphUtils::AddEdge(floor_node1->GetOutDataAnchor(0), floor_node2->GetInDataAnchor(0)),
            ge::GRAPH_SUCCESS);

  PrunePass prune_pass;
  vector<GraphPass *> passes = {&prune_pass};
}

// case 4: multi net path with one common netout(1:multi：1)
TEST_F(UtestGraphPassesPrunePass, has_net_out_put_node_with_multi_path) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("default");

  ge::OpDescPtr data_op = std::make_shared<ge::OpDesc>();
  ge::OpDescUtilsEx::SetType(data_op, DATA);
  data_op->SetName("data");
  data_op->AddOutputDesc(ge::GeTensorDesc());
  data_op->AddOutputDesc(ge::GeTensorDesc());
  data_op->AddOutputDesc(ge::GeTensorDesc());
  ge::NodePtr data_node = graph->AddNode(data_op);

  ge::OpDescPtr reverse_op1 = std::make_shared<ge::OpDesc>();
  ge::OpDescUtilsEx::SetType(reverse_op1, REVERSE);
  reverse_op1->SetName("Reverse1");
  reverse_op1->AddInputDesc(ge::GeTensorDesc());
  reverse_op1->AddOutputDesc(ge::GeTensorDesc());
  ge::NodePtr reverse_node1 = graph->AddNode(reverse_op1);

  ge::OpDescPtr floor_op1 = std::make_shared<ge::OpDesc>();
  ge::OpDescUtilsEx::SetType(floor_op1, FLOOR);
  floor_op1->SetName("Floor1");
  floor_op1->AddInputDesc(ge::GeTensorDesc());
  floor_op1->AddOutputDesc(ge::GeTensorDesc());
  ge::NodePtr floor_node1 = graph->AddNode(floor_op1);

  ge::OpDescPtr reverse_op2 = std::make_shared<ge::OpDesc>();
  ge::OpDescUtilsEx::SetType(reverse_op2, REVERSE);
  reverse_op2->SetName("Reverse2");
  reverse_op2->AddInputDesc(ge::GeTensorDesc());
  reverse_op2->AddOutputDesc(ge::GeTensorDesc());
  ge::NodePtr reverse_node2 = graph->AddNode(reverse_op2);

  ge::OpDescPtr floor_op2 = std::make_shared<ge::OpDesc>();
  ge::OpDescUtilsEx::SetType(floor_op2, FLOOR);
  floor_op2->SetName("Floor2");
  floor_op2->AddInputDesc(ge::GeTensorDesc());
  floor_op2->AddOutputDesc(ge::GeTensorDesc());
  ge::NodePtr floor_node2 = graph->AddNode(floor_op2);

  ge::OpDescPtr reverse_op3 = std::make_shared<ge::OpDesc>();
  ge::OpDescUtilsEx::SetType(reverse_op3, REVERSE);
  reverse_op3->SetName("Reverse3");
  reverse_op3->AddInputDesc(ge::GeTensorDesc());
  reverse_op3->AddOutputDesc(ge::GeTensorDesc());
  ge::NodePtr reverse_node3 = graph->AddNode(reverse_op3);

  ge::OpDescPtr floor_op3 = std::make_shared<ge::OpDesc>();
  ge::OpDescUtilsEx::SetType(floor_op3, FLOOR);
  floor_op3->SetName("Floor3");
  floor_op3->AddInputDesc(ge::GeTensorDesc());
  floor_op3->AddOutputDesc(ge::GeTensorDesc());
  ge::NodePtr floor_node3 = graph->AddNode(floor_op3);

  ge::OpDescPtr net_output_op = std::make_shared<ge::OpDesc>(NODE_NAME_NET_OUTPUT, NETOUTPUT);
  net_output_op->AddInputDesc(ge::GeTensorDesc());
  net_output_op->AddInputDesc(ge::GeTensorDesc());
  net_output_op->AddInputDesc(ge::GeTensorDesc());
  net_output_op->AddOutputDesc(ge::GeTensorDesc());
  ge::AttrUtils::SetBool(net_output_op, "identity_add_netoutput", true);
  ge::NodePtr netoutput_node = graph->AddNode(net_output_op);

  ge::GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), reverse_node1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(data_node->GetOutDataAnchor(1), reverse_node2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(data_node->GetOutDataAnchor(2), reverse_node3->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(reverse_node1->GetOutDataAnchor(0), floor_node1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(floor_node1->GetOutDataAnchor(0), netoutput_node->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(reverse_node2->GetOutDataAnchor(0), floor_node2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(floor_node2->GetOutDataAnchor(0), netoutput_node->GetInDataAnchor(1));
  ge::GraphUtils::AddEdge(reverse_node3->GetOutDataAnchor(0), floor_node3->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(floor_node3->GetOutDataAnchor(0), netoutput_node->GetInDataAnchor(2));

  uint64_t size_ori = graph->GetDirectNode().size();

  PrunePass prune_pass;
  std::vector<std::pair<string, GraphPass *>> passes = {{"PrunePass", &prune_pass}};
  (void)PassManager::Run(graph, passes);

  uint64_t size_after_proc = graph->GetDirectNode().size();
  EXPECT_TRUE(size_ori == size_after_proc);
}
// case 5: circle,diamand style
TEST_F(UtestGraphPassesPrunePass, multi_net_out_put_node_with_circle_net) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("default");

  ge::OpDescPtr data_op = std::make_shared<ge::OpDesc>();
  ge::OpDescUtilsEx::SetType(data_op, DATA);
  data_op->SetName("data");
  data_op->AddOutputDesc(ge::GeTensorDesc());
  data_op->AddOutputDesc(ge::GeTensorDesc());
  ge::NodePtr data_node = graph->AddNode(data_op);

  ge::OpDescPtr op_1 = std::make_shared<ge::OpDesc>();
  ge::OpDescUtilsEx::SetType(op_1, REVERSE);
  op_1->SetName("Reverse1");
  op_1->AddInputDesc(ge::GeTensorDesc());
  op_1->AddOutputDesc(ge::GeTensorDesc());
  op_1->AddOutputDesc(ge::GeTensorDesc());
  ge::NodePtr node_1 = graph->AddNode(op_1);

  ge::OpDescPtr op_2 = std::make_shared<ge::OpDesc>();
  ge::OpDescUtilsEx::SetType(op_2, REVERSE);
  op_2->SetName("Reverse2");
  op_2->AddInputDesc(ge::GeTensorDesc());
  op_2->AddInputDesc(ge::GeTensorDesc());
  op_2->AddOutputDesc(ge::GeTensorDesc());
  ge::NodePtr node_2 = graph->AddNode(op_2);

  ge::OpDescPtr op_3 = std::make_shared<ge::OpDesc>();
  ge::OpDescUtilsEx::SetType(op_3, REVERSE);
  op_3->SetName("Reverse3");
  op_3->AddInputDesc(ge::GeTensorDesc());
  op_3->AddInputDesc(ge::GeTensorDesc());
  op_3->AddOutputDesc(ge::GeTensorDesc());
  ge::NodePtr node_3 = graph->AddNode(op_3);

  ge::OpDescPtr op_4 = std::make_shared<ge::OpDesc>();
  ge::OpDescUtilsEx::SetType(op_4, REVERSE);
  op_4->SetName("Reverse4");
  op_4->AddInputDesc(ge::GeTensorDesc());
  op_4->AddOutputDesc(ge::GeTensorDesc());
  ge::NodePtr node_4 = graph->AddNode(op_4);

  ge::OpDescPtr op_5 = std::make_shared<ge::OpDesc>();
  ge::OpDescUtilsEx::SetType(op_5, REVERSE);
  op_5->SetName("Reverse5");
  op_5->AddInputDesc(ge::GeTensorDesc());
  op_5->AddOutputDesc(ge::GeTensorDesc());
  ge::NodePtr node_5 = graph->AddNode(op_5);

  ge::OpDescPtr net_output_op = std::make_shared<ge::OpDesc>(NODE_NAME_NET_OUTPUT, NETOUTPUT);
  net_output_op->AddInputDesc(ge::GeTensorDesc());
  net_output_op->AddOutputDesc(ge::GeTensorDesc());
  ge::AttrUtils::SetBool(net_output_op, "identity_add_netoutput", true);
  ge::NodePtr netoutput_node = graph->AddNode(net_output_op);

  ge::GraphUtils::AddEdge(node_1->GetOutDataAnchor(0), netoutput_node->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_2->GetOutDataAnchor(0), node_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_3->GetOutDataAnchor(0), node_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4->GetOutDataAnchor(0), node_3->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_1->GetOutDataAnchor(1), node_4->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), node_2->GetInDataAnchor(1));
  ge::GraphUtils::AddEdge(data_node->GetOutDataAnchor(1), node_5->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_5->GetOutDataAnchor(0), node_3->GetInDataAnchor(1));

  uint64_t size_ori = graph->GetDirectNode().size();

  PrunePass prune_pass;
  std::vector<std::pair<string, GraphPass *>> passes = {{"PrunePass", &prune_pass}};
  Status status = PassManager::Run(graph, passes);
  EXPECT_EQ(ge::SUCCESS, status);
  uint64_t size_after_proc = graph->GetDirectNode().size();
  EXPECT_TRUE(size_ori == size_after_proc);
}

// case 6: two mix circle and multi path,diamand style
TEST_F(UtestGraphPassesPrunePass, mix_two_circle_net) {
  EXPECT_NO_THROW(
      ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("default");

      ge::OpDescPtr data_op = std::make_shared<ge::OpDesc>(); ge::OpDescUtilsEx::SetType(data_op, DATA);
      data_op->SetName("data"); data_op->AddOutputDesc(ge::GeTensorDesc()); data_op->AddOutputDesc(ge::GeTensorDesc());
      ge::NodePtr data_node = graph->AddNode(data_op);

      ge::OpDescPtr op_1 = std::make_shared<ge::OpDesc>(); ge::OpDescUtilsEx::SetType(op_1, REVERSE);
      op_1->SetName("Reverse1"); op_1->AddInputDesc(ge::GeTensorDesc()); op_1->AddInputDesc(ge::GeTensorDesc());
      op_1->AddOutputDesc(ge::GeTensorDesc()); ge::NodePtr node_1 = graph->AddNode(op_1);

      ge::OpDescPtr op_2 = std::make_shared<ge::OpDesc>(); ge::OpDescUtilsEx::SetType(op_2, REVERSE);
      op_2->SetName("Reverse2"); op_2->AddInputDesc(ge::GeTensorDesc()); op_2->AddOutputDesc(ge::GeTensorDesc());
      op_2->AddOutputDesc(ge::GeTensorDesc()); ge::NodePtr node_2 = graph->AddNode(op_2);

      ge::OpDescPtr op_3 = std::make_shared<ge::OpDesc>(); ge::OpDescUtilsEx::SetType(op_3, REVERSE);
      op_3->SetName("Reverse3"); op_3->AddInputDesc(ge::GeTensorDesc()); op_3->AddInputDesc(ge::GeTensorDesc());
      op_3->AddOutputDesc(ge::GeTensorDesc()); ge::NodePtr node_3 = graph->AddNode(op_3);

      ge::OpDescPtr op_4 = std::make_shared<ge::OpDesc>(); ge::OpDescUtilsEx::SetType(op_4, REVERSE);
      op_4->SetName("Reverse4"); op_4->AddInputDesc(ge::GeTensorDesc()); op_4->AddInputDesc(ge::GeTensorDesc());
      op_4->AddOutputDesc(ge::GeTensorDesc()); ge::NodePtr node_4 = graph->AddNode(op_4);

      ge::OpDescPtr op_5 = std::make_shared<ge::OpDesc>(); ge::OpDescUtilsEx::SetType(op_5, REVERSE);
      op_5->SetName("Reverse5"); op_5->AddInputDesc(ge::GeTensorDesc()); op_5->AddOutputDesc(ge::GeTensorDesc());
      op_5->AddOutputDesc(ge::GeTensorDesc()); ge::NodePtr node_5 = graph->AddNode(op_5);

      ge::OpDescPtr net_output_op = std::make_shared<ge::OpDesc>(NODE_NAME_NET_OUTPUT, NETOUTPUT);
      net_output_op->AddInputDesc(ge::GeTensorDesc()); net_output_op->AddOutputDesc(ge::GeTensorDesc());
      ge::AttrUtils::SetBool(net_output_op, "identity_add_netoutput", true);
      ge::NodePtr netoutput_node = graph->AddNode(net_output_op);

      ge::GraphUtils::AddEdge(node_1->GetOutDataAnchor(0), netoutput_node->GetInDataAnchor(0));
      ge::GraphUtils::AddEdge(node_2->GetOutDataAnchor(0), node_1->GetInDataAnchor(0));
      ge::GraphUtils::AddEdge(node_5->GetOutDataAnchor(0), node_1->GetInDataAnchor(1));
      ge::GraphUtils::AddEdge(node_4->GetOutDataAnchor(0), node_2->GetInDataAnchor(0));
      ge::GraphUtils::AddEdge(node_2->GetOutDataAnchor(1), node_3->GetInDataAnchor(0));
      ge::GraphUtils::AddEdge(node_5->GetOutDataAnchor(1), node_3->GetInDataAnchor(1));
      ge::GraphUtils::AddEdge(node_3->GetOutDataAnchor(0), node_4->GetInDataAnchor(0));
      ge::GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), node_4->GetInDataAnchor(1));
      ge::GraphUtils::AddEdge(node_4->GetOutDataAnchor(1), node_5->GetInDataAnchor(0));
      // construct two isolated node
      ge::OpDescPtr op_6 = std::make_shared<ge::OpDesc>();
      ge::OpDescUtilsEx::SetType(op_6, REVERSE); op_6->SetName("Reverse"); op_6->AddInputDesc(ge::GeTensorDesc());
      op_6->AddOutputDesc(ge::GeTensorDesc()); ge::NodePtr node_6 = graph->AddNode(op_6);

      ge::OpDescPtr op_7 = std::make_shared<ge::OpDesc>(); ge::OpDescUtilsEx::SetType(op_7, REVERSE);
      op_7->SetName("Reverse"); op_7->AddInputDesc(ge::GeTensorDesc()); op_7->AddOutputDesc(ge::GeTensorDesc());
      ge::NodePtr node_7 = graph->AddNode(op_7);

      PrunePass prune_pass; vector<GraphPass *> passes = {&prune_pass};);
}
// case7: one net path with two DATA node
TEST_F(UtestGraphPassesPrunePass, has_net_out_put_node_with_two_isolate_data_node) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("default");

  ge::OpDescPtr reverse_op = std::make_shared<ge::OpDesc>();
  ge::OpDescUtilsEx::SetType(reverse_op, REVERSE);
  reverse_op->SetName("Reverse");
  reverse_op->AddOutputDesc(ge::GeTensorDesc());
  ge::NodePtr reverse_node = graph->AddNode(reverse_op);

  ge::OpDescPtr floor_op = std::make_shared<ge::OpDesc>();
  ge::OpDescUtilsEx::SetType(floor_op, FLOOR);
  floor_op->SetName("Floor");
  floor_op->AddInputDesc(ge::GeTensorDesc());
  floor_op->AddOutputDesc(ge::GeTensorDesc());
  ge::NodePtr floor_node = graph->AddNode(floor_op);

  ge::OpDescPtr net_output_op = std::make_shared<ge::OpDesc>(NODE_NAME_NET_OUTPUT, NETOUTPUT);
  net_output_op->AddInputDesc(ge::GeTensorDesc());
  net_output_op->AddOutputDesc(ge::GeTensorDesc());
  ge::AttrUtils::SetBool(net_output_op, "identity_add_netoutput", true);
  ge::NodePtr netoutput_node = graph->AddNode(net_output_op);
  // construct one isolated DATA node (to be deleted)
  ge::OpDescPtr reverse_op_1 = std::make_shared<ge::OpDesc>();
  ge::OpDescUtilsEx::SetType(reverse_op_1, REVERSE);
  reverse_op_1->SetName("Reverse1");
  reverse_op_1->AddOutputDesc(ge::GeTensorDesc());
  ge::NodePtr reverse_node_1 = graph->AddNode(reverse_op_1);

  ge::GraphUtils::AddEdge(reverse_node->GetOutDataAnchor(0), floor_node->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(floor_node->GetOutDataAnchor(0), netoutput_node->GetInDataAnchor(0));
  // construct two isolated DATA nodes(to be not deleted)
  ge::OpDescPtr data_op_1 = std::make_shared<ge::OpDesc>();
  ge::OpDescUtilsEx::SetType(data_op_1, DATA);
  data_op_1->SetName("data");
  data_op_1->AddOutputDesc(ge::GeTensorDesc());
  data_op_1->AddOutputDesc(ge::GeTensorDesc());
  ge::NodePtr data_node_1 = graph->AddNode(data_op_1);

  ge::OpDescPtr data_op_2 = std::make_shared<ge::OpDesc>();
  ge::OpDescUtilsEx::SetType(data_op_2, DATA);
  data_op_2->SetName("data1");
  data_op_2->AddOutputDesc(ge::GeTensorDesc());
  data_op_2->AddOutputDesc(ge::GeTensorDesc());
  ge::NodePtr data_node = graph->AddNode(data_op_2);

  uint64_t size_ori = graph->GetDirectNode().size();
  PrunePass prune_pass;
  std::vector<std::pair<string, GraphPass *>> passes = {{"PrunePass", &prune_pass}};
  Status status = PassManager::Run(graph, passes);

  uint64_t size = graph->GetDirectNode().size();
  EXPECT_EQ(ge::SUCCESS, status);
  EXPECT_EQ(size_ori, (size + 1));

  // it should check net_out_put's input data node and input control node
  auto control_vec = netoutput_node->GetInControlNodes();
  EXPECT_EQ(control_vec.size(), 2);
  // check control_vec contains only data node
  for (auto node : control_vec) {
    bool result = (node->GetName() == "data" || node->GetName() == "data1") ? true : false;
    EXPECT_EQ(result, true);
  }

  auto data_vec = netoutput_node->GetInDataNodes();
  EXPECT_EQ(data_vec.size(), 1);
  // check data_vec contains only Floor node
  for (auto node : data_vec) {
    bool result = (node->GetName() == "Floor") ? true : false;
    EXPECT_EQ(result, true);
  }
}

TEST_F(UtestGraphPassesPrunePass, skip_do_prune_with_skip_attr) {
  DEF_GRAPH(g1) {
    CHAIN(NODE("data1", "Data")->NODE("matmul1", "Matmul")->NODE("NetOutput", "NetOutput"));
    CHAIN(NODE("const1", "Constant")->EDGE(0, 1)->NODE("matmul1"));
    CHAIN(NODE("const1", "Constant")->EDGE(0, 0)->NODE("cmo1", "Cmo"));
    CHAIN(NODE("const2", "Constant")->EDGE(0, 0)->NODE("cmo2", "Cmo"));
    CHAIN(NODE("const3", "Constant")->EDGE(0, 0)->NODE("cmo3", "Cmo"));
  };
  auto graph = ToComputeGraph(g1);
  ASSERT_NE(graph, nullptr);
  auto cmo1 = graph->FindNode("cmo1");
  ASSERT_NE(cmo1, nullptr);
  AttrUtils::SetBool(cmo1->GetOpDescBarePtr(), ATTR_NAME_SKIP_PRUNE_OPTIMIZE, true);
  auto cmo2 = graph->FindNode("cmo2");
  AttrUtils::SetBool(cmo2->GetOpDescBarePtr(), ATTR_NAME_SKIP_PRUNE_OPTIMIZE, true);
  auto cmo3 = graph->FindNode("cmo3");

  PrunePass prune_pass;
  std::vector<std::pair<string, GraphPass *>> passes = {{"PrunePass", &prune_pass}};
  Status status = PassManager::Run(graph, passes);
  EXPECT_EQ(ge::SUCCESS, status);
  auto cmo1_res = graph->FindNode("cmo1");
  EXPECT_NE(cmo1_res, nullptr);
  auto cmo2_res = graph->FindNode("cmo2");

  EXPECT_NE(cmo2_res, nullptr);
  auto cmo3_res = graph->FindNode("cmo3");
  EXPECT_EQ(cmo3_res, nullptr);
  auto const1_res = graph->FindNode("const1");
  EXPECT_NE(const1_res, nullptr);
  auto const2_res = graph->FindNode("const2");
  EXPECT_NE(const2_res, nullptr);
  auto const3_res = graph->FindNode("const3");
  EXPECT_EQ(const3_res, nullptr);
}

TEST_F(UtestGraphPassesPrunePass, RefNodeWithoutOutput_Skip_prune) {
  // build sub graph
  auto graph_builder = ut::GraphBuilder("test");
  auto data_1 = graph_builder.AddNode("data_1", DATA, 0, 1);
  auto data_2 = graph_builder.AddNode("data_2", DATA, 0, 1);
  auto assign = graph_builder.AddNode("assign", ASSIGN, 2, 1);
  AttrUtils::SetBool(assign->GetOpDesc(), ATTR_NAME_REFERENCE, true);
  auto out_0 = graph_builder.AddNode("out", NETOUTPUT, 1, 1);
  graph_builder.AddDataEdge(data_1, 0, assign, 0);
  graph_builder.AddDataEdge(data_1, 0, assign, 1);

  auto graph = graph_builder.GetGraph();
  auto size_ori = graph->GetDirectNodesSize();
  PrunePass prune_pass;
  std::vector<std::pair<string, GraphPass *>> passes = {{"PrunePass", &prune_pass}};
  Status status = PassManager::Run(graph, passes);
  uint64_t size = graph->GetDirectNode().size();
  EXPECT_EQ(ge::SUCCESS, status);
  EXPECT_EQ(size_ori, size);
}

TEST_F(UtestGraphPassesPrunePass, null_graph) {
  PrunePass prune_pass;
  ge::ComputeGraphPtr null_graph = nullptr;
  EXPECT_EQ(prune_pass.Run(null_graph), GE_GRAPH_ISNULL);
}

TEST_F(UtestGraphPassesPrunePass, empty_graph_no_nodes) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("empty");
  PrunePass prune_pass;
  std::vector<std::pair<string, GraphPass *>> passes = {{"PrunePass", &prune_pass}};
  Status status = PassManager::Run(graph, passes);
  EXPECT_EQ(ge::SUCCESS, status);
  EXPECT_EQ(graph->GetDirectNode().size(), 0);
}

TEST_F(UtestGraphPassesPrunePass, standard_prune_preserves_completion_edge) {
  ut::GraphBuilder builder("standard");
  auto data = builder.AddNode("data", DATA, 0, 1);
  auto dead = builder.AddNode("dead", RELU, 1, 1);
  auto output = builder.AddNode("output", NETOUTPUT, 0, 0);
  builder.AddDataEdge(data, 0, dead, 0);
  builder.AddControlEdge(dead, output);
  auto graph = builder.GetGraph();

  ASSERT_EQ(PrunePass().Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), 3U);
  EXPECT_TRUE(dead->GetOutControlAnchor()->IsLinkedWith(output->GetInControlAnchor()));
}

TEST_F(UtestGraphPassesPrunePass, after_engine_prune_removes_chain_and_restores_keep_alive_completion) {
  ut::GraphBuilder builder("dead_chain");
  auto data = builder.AddNode("data", DATA, 0, 1);
  auto live = builder.AddNode("live", RELU, 1, 1);
  auto weight = builder.AddNode("weight", DATA, 0, 1);
  auto keep = builder.AddNode("keep", "HcomAllReduce", 1, 1);
  auto dead1 = builder.AddNode("dead1", MUL, 2, 1);
  auto dead2 = builder.AddNode("dead2", RELU, 1, 1);
  auto output = builder.AddNode("output", NETOUTPUT, 1, 0);
  keep->GetOpDesc()->SetOpEngineName("DNN_HCCL");
  keep->GetOpDesc()->SetOpKernelLibName("different_hccl_kernel_library");
  builder.AddDataEdge(data, 0, live, 0);
  builder.AddDataEdge(live, 0, output, 0);
  builder.AddDataEdge(live, 0, dead1, 0);
  builder.AddDataEdge(weight, 0, keep, 0);
  builder.AddDataEdge(keep, 0, dead1, 1);
  builder.AddDataEdge(dead1, 0, dead2, 0);
  builder.AddControlEdge(dead2, output);
  auto graph = builder.GetGraph();
  PrunePass pass(PrunePass::Stage::kAfterEngineAssigned);

  ASSERT_EQ(pass.Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), 5U);
  EXPECT_EQ(graph->FindNode("dead1"), nullptr);
  EXPECT_EQ(graph->FindNode("dead2"), nullptr);
  EXPECT_TRUE(live->GetOutDataAnchor(0)->IsLinkedWith(output->GetInDataAnchor(0)));
  EXPECT_FALSE(live->GetOutControlAnchor()->IsLinkedWith(output->GetInControlAnchor()));
  EXPECT_TRUE(keep->GetOutControlAnchor()->IsLinkedWith(output->GetInControlAnchor()));
  EXPECT_TRUE(weight->GetOutDataAnchor(0)->IsLinkedWith(keep->GetInDataAnchor(0)));
  EXPECT_EQ(output->GetInControlNodesSize(), 1U);
  ASSERT_EQ(pass.Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), 5U);
  EXPECT_EQ(output->GetInControlNodesSize(), 1U);
}

TEST_F(UtestGraphPassesPrunePass, after_engine_prune_restores_each_original_output) {
  ut::GraphBuilder builder("multiple_outputs");
  auto shared = builder.AddNode("shared", "HcomAllReduce", 0, 1);
  auto keep1 = builder.AddNode("keep1", "HcomAllReduce", 0, 1);
  auto keep2 = builder.AddNode("keep2", "HcomAllReduce", 0, 1);
  auto dead1 = builder.AddNode("dead1", MUL, 2, 1);
  auto dead2 = builder.AddNode("dead2", MUL, 2, 1);
  auto output1 = builder.AddNode("output1", NETOUTPUT, 0, 0);
  auto output2 = builder.AddNode("output2", NETOUTPUT, 0, 0);
  shared->GetOpDesc()->SetOpEngineName("DNN_HCCL");
  keep1->GetOpDesc()->SetOpEngineName("DNN_HCCL");
  keep2->GetOpDesc()->SetOpEngineName("DNN_HCCL");
  builder.AddDataEdge(shared, 0, dead1, 1);
  builder.AddDataEdge(shared, 0, dead2, 1);
  builder.AddDataEdge(keep1, 0, dead1, 0);
  builder.AddDataEdge(keep2, 0, dead2, 0);
  builder.AddControlEdge(dead1, output1);
  builder.AddControlEdge(dead2, output2);
  auto graph = builder.GetGraph();

  ASSERT_EQ(PrunePass(PrunePass::Stage::kAfterEngineAssigned).Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), 5U);
  EXPECT_TRUE(shared->GetOutControlAnchor()->IsLinkedWith(output1->GetInControlAnchor()));
  EXPECT_TRUE(shared->GetOutControlAnchor()->IsLinkedWith(output2->GetInControlAnchor()));
  EXPECT_TRUE(keep1->GetOutControlAnchor()->IsLinkedWith(output1->GetInControlAnchor()));
  EXPECT_FALSE(keep1->GetOutControlAnchor()->IsLinkedWith(output2->GetInControlAnchor()));
  EXPECT_TRUE(keep2->GetOutControlAnchor()->IsLinkedWith(output2->GetInControlAnchor()));
  EXPECT_FALSE(keep2->GetOutControlAnchor()->IsLinkedWith(output1->GetInControlAnchor()));
}

TEST_F(UtestGraphPassesPrunePass, after_engine_prune_keeps_completion_when_source_is_already_live) {
  ut::GraphBuilder builder("live_source");
  auto source = builder.AddNode("source", RELU, 0, 1);
  auto output1 = builder.AddNode("output1", NETOUTPUT, 0, 0);
  auto output2 = builder.AddNode("output2", NETOUTPUT, 1, 0);
  builder.AddControlEdge(source, output1);
  builder.AddDataEdge(source, 0, output2, 0);
  auto graph = builder.GetGraph();

  ASSERT_EQ(PrunePass(PrunePass::Stage::kAfterEngineAssigned).Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), 3U);
  EXPECT_TRUE(source->GetOutControlAnchor()->IsLinkedWith(output1->GetInControlAnchor()));
  EXPECT_TRUE(source->GetOutDataAnchor(0)->IsLinkedWith(output2->GetInDataAnchor(0)));
}

TEST_F(UtestGraphPassesPrunePass, after_engine_prune_removes_control_only_computation_without_attribute) {
  ut::GraphBuilder builder("completion_only");
  auto data = builder.AddNode("data", DATA, 0, 1);
  auto compute = builder.AddNode("compute", RELU, 1, 1);
  auto output = builder.AddNode("output", NETOUTPUT, 0, 0);
  builder.AddDataEdge(data, 0, compute, 0);
  builder.AddControlEdge(compute, output);
  auto graph = builder.GetGraph();

  ASSERT_EQ(PrunePass(PrunePass::Stage::kAfterEngineAssigned).Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), 2U);
  EXPECT_EQ(graph->FindNode("compute"), nullptr);
  EXPECT_EQ(data->GetOutDataAnchor(0)->GetPeerAnchorsSize(), 0U);
  EXPECT_TRUE(data->GetOutControlAnchor()->IsLinkedWith(output->GetInControlAnchor()));
}

TEST_F(UtestGraphPassesPrunePass, after_engine_prune_keeps_independent_keep_alive_input_closure) {
  ut::GraphBuilder builder("independent_keep_alive");
  auto data = builder.AddNode("data", DATA, 0, 1);
  auto input = builder.AddNode("input", RELU, 1, 1);
  auto keep = builder.AddNode("keep", "KeptByEngine", 1, 1);
  auto output = builder.AddNode("output", NETOUTPUT, 0, 0);
  keep->GetOpDesc()->SetOpEngineName("DNN_VM_RTS");
  keep->GetOpDesc()->SetOpKernelLibName("another_rts_library");
  builder.AddDataEdge(data, 0, input, 0);
  builder.AddDataEdge(input, 0, keep, 0);
  auto graph = builder.GetGraph();

  ASSERT_EQ(PrunePass(PrunePass::Stage::kAfterEngineAssigned).Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), 4U);
  EXPECT_NE(graph->FindNode("input"), nullptr);
  EXPECT_TRUE(input->GetOutDataAnchor(0)->IsLinkedWith(keep->GetInDataAnchor(0)));
  EXPECT_EQ(output->GetInControlNodesSize(), 0U);
}

TEST_F(UtestGraphPassesPrunePass, after_engine_prune_does_not_delete_protected_constant_with_dead_consumer) {
  ut::GraphBuilder builder("protected_constant");
  auto constant = builder.AddNode("constant", CONSTANT, 0, 1);
  auto dead = builder.AddNode("dead", RELU, 1, 1);
  auto output = builder.AddNode("output", NETOUTPUT, 0, 0);
  ASSERT_TRUE(AttrUtils::SetBool(constant->GetOpDesc(), ATTR_NAME_SKIP_PRUNE_OPTIMIZE, true));
  builder.AddDataEdge(constant, 0, dead, 0);
  auto graph = builder.GetGraph();

  ASSERT_EQ(PrunePass(PrunePass::Stage::kAfterEngineAssigned).Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), 2U);
  EXPECT_EQ(graph->FindNode("constant"), constant);
  EXPECT_EQ(graph->FindNode("dead"), nullptr);
  EXPECT_EQ(constant->GetOutDataAnchor(0)->GetPeerAnchorsSize(), 0U);
  EXPECT_EQ(output->GetInControlNodesSize(), 0U);
}

TEST_F(UtestGraphPassesPrunePass, after_engine_prune_preserves_shared_constant_live_consumer) {
  ut::GraphBuilder builder("shared_constant");
  auto constant = builder.AddNode("constant", CONSTANT, 0, 1);
  auto live = builder.AddNode("live", RELU, 1, 1);
  auto dead = builder.AddNode("dead", RELU, 1, 1);
  auto output = builder.AddNode("output", NETOUTPUT, 1, 0);
  builder.AddDataEdge(constant, 0, live, 0);
  builder.AddDataEdge(constant, 0, dead, 0);
  builder.AddDataEdge(live, 0, output, 0);
  builder.AddControlEdge(dead, output);
  auto graph = builder.GetGraph();

  ASSERT_EQ(PrunePass(PrunePass::Stage::kAfterEngineAssigned).Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), 3U);
  EXPECT_EQ(graph->FindNode("dead"), nullptr);
  EXPECT_TRUE(constant->GetOutDataAnchor(0)->IsLinkedWith(live->GetInDataAnchor(0)));
  EXPECT_TRUE(live->GetOutDataAnchor(0)->IsLinkedWith(output->GetInDataAnchor(0)));
}

TEST_F(UtestGraphPassesPrunePass, after_engine_prune_preserves_explicit_target_and_skip_and_reference) {
  ut::GraphBuilder builder("protected_sources");
  auto target = builder.AddNode("target", RELU, 0, 1);
  auto skip = builder.AddNode("skip", RELU, 0, 1);
  auto reference = builder.AddNode("reference", RELU, 0, 1);
  auto print = builder.AddNode("print", "Print", 0, 0);
  auto print_v2 = builder.AddNode("print_v2", "PrintV2", 0, 0);
  auto assertion = builder.AddNode("assertion", "Assert", 0, 0);
  auto output = builder.AddNode("output", NETOUTPUT, 0, 0);
  builder.AddControlEdge(target, output);
  builder.AddControlEdge(skip, output);
  builder.AddControlEdge(reference, output);
  builder.AddControlEdge(print, output);
  builder.AddControlEdge(print_v2, output);
  builder.AddControlEdge(assertion, output);
  ASSERT_TRUE(AttrUtils::SetBool(skip->GetOpDesc(), ATTR_NAME_SKIP_PRUNE_OPTIMIZE, true));
  ASSERT_TRUE(AttrUtils::SetBool(reference->GetOpDesc(), ATTR_NAME_REFERENCE, true));
  auto graph = builder.GetGraph();
  graph->SetGraphTargetNodesInfo({target});

  ASSERT_EQ(PrunePass(PrunePass::Stage::kAfterEngineAssigned).Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), 7U);
  EXPECT_EQ(output->GetInControlNodesSize(), 6U);
}

TEST_F(UtestGraphPassesPrunePass, after_engine_prune_preserves_subgraph_call_and_inner_target_completion) {
  ut::GraphBuilder builder("parent");
  auto call = builder.AddNode("call", "CustomCall", 0, 1);
  auto output = builder.AddNode("output", NETOUTPUT, 0, 0);
  builder.AddControlEdge(call, output);
  auto graph = builder.GetGraph();
  ut::GraphBuilder sub_builder("body");
  auto compute = sub_builder.AddNode("compute", RELU, 0, 1);
  auto inner_output = sub_builder.AddNode("inner_output", NETOUTPUT, 0, 0);
  sub_builder.AddControlEdge(compute, inner_output);
  auto subgraph = sub_builder.GetGraph();
  subgraph->SetGraphTargetNodesInfo({compute});
  ut::GraphBuilder::AddPartitionedCall(graph, "call", subgraph);
  PrunePass pass(PrunePass::Stage::kAfterEngineAssigned);
  std::vector<std::pair<string, GraphPass *>> passes = {{"PrunePass", &pass}};

  ASSERT_EQ(PassManager::Run(graph, passes), SUCCESS);
  EXPECT_EQ(graph->FindNode("call"), call);
  EXPECT_EQ(graph->GetSubgraph("body"), subgraph);
  EXPECT_TRUE(call->GetOutControlAnchor()->IsLinkedWith(output->GetInControlAnchor()));
  EXPECT_EQ(subgraph->FindNode("compute"), compute);
  EXPECT_TRUE(compute->GetOutControlAnchor()->IsLinkedWith(inner_output->GetInControlAnchor()));
}

TEST_F(UtestGraphPassesPrunePass, after_engine_prune_tracks_mixed_input_dependency) {
  ut::GraphBuilder builder("mixed_live_input");
  auto producer = builder.AddNode("producer", RELU, 0, 1);
  auto output = builder.AddNode("output", NETOUTPUT, 0, 0);
  auto graph = builder.GetGraph();
  ASSERT_EQ(GraphUtils::AddEdge(producer->GetOutDataAnchor(0), output->GetInControlAnchor()), GRAPH_SUCCESS);

  ASSERT_EQ(PrunePass(PrunePass::Stage::kAfterEngineAssigned).Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), 2U);
  EXPECT_EQ(graph->FindNode("producer"), producer);
  EXPECT_TRUE(producer->GetOutDataAnchor(0)->IsLinkedWith(output->GetInControlAnchor()));
}

TEST_F(UtestGraphPassesPrunePass, after_engine_prune_restores_boundary_through_mixed_input) {
  ut::GraphBuilder builder("mixed_dead_input");
  auto data = builder.AddNode("data", DATA, 0, 1);
  auto dead = builder.AddNode("dead", RELU, 0, 1);
  auto output = builder.AddNode("output", NETOUTPUT, 0, 0);
  builder.AddControlEdge(dead, output);
  auto graph = builder.GetGraph();
  ASSERT_EQ(GraphUtils::AddEdge(data->GetOutDataAnchor(0), dead->GetInControlAnchor()), GRAPH_SUCCESS);

  ASSERT_EQ(PrunePass(PrunePass::Stage::kAfterEngineAssigned).Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), 2U);
  EXPECT_EQ(graph->FindNode("dead"), nullptr);
  EXPECT_TRUE(data->GetOutControlAnchor()->IsLinkedWith(output->GetInControlAnchor()));
  EXPECT_EQ(data->GetOutDataAnchor(0)->GetPeerAnchorsSize(), 0U);
}

TEST_F(UtestGraphPassesPrunePass, after_engine_prune_cleans_unreachable_nodes_without_candidates) {
  ut::GraphBuilder builder("unreachable");
  auto data = builder.AddNode("data", DATA, 0, 1);
  auto dead1 = builder.AddNode("dead1", RELU, 1, 1);
  auto dead2 = builder.AddNode("dead2", RELU, 1, 1);
  auto output = builder.AddNode("output", NETOUTPUT, 0, 0);
  builder.AddDataEdge(data, 0, dead1, 0);
  builder.AddDataEdge(dead1, 0, dead2, 0);
  auto graph = builder.GetGraph();

  ASSERT_EQ(PrunePass(PrunePass::Stage::kAfterEngineAssigned).Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), 2U);
  EXPECT_EQ(graph->FindNode("data"), data);
  EXPECT_EQ(graph->FindNode("dead1"), nullptr);
  EXPECT_EQ(graph->FindNode("dead2"), nullptr);
  EXPECT_EQ(output->GetInControlNodesSize(), 0U);
}

TEST_F(UtestGraphPassesPrunePass, after_engine_prune_does_nothing_without_netoutput) {
  ut::GraphBuilder builder("no_output");
  auto compute = builder.AddNode("compute", RELU, 0, 1);
  auto graph = builder.GetGraph();
  PrunePass pass(PrunePass::Stage::kAfterEngineAssigned);

  ASSERT_EQ(pass.Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), 1U);
  EXPECT_EQ(graph->FindNode("compute"), compute);
  auto empty = std::make_shared<ComputeGraph>("empty");
  EXPECT_EQ(pass.Run(empty), SUCCESS);
  EXPECT_EQ(pass.Run(nullptr), GE_GRAPH_ISNULL);
}

// 无执行标签时不因 Enter 类型跳过剪枝，自动删除冗余分支并恢复引擎保活节点的完成依赖。
TEST_F(UtestGraphPassesPrunePass, after_engine_prune_restores_engine_completion_without_execution_label) {
  ut::GraphBuilder builder("legacy_control_flow");
  auto data = builder.AddNode("data", DATA, 0, 1);
  auto enter = builder.AddNode("enter", ENTER, 1, 1);
  auto compute = builder.AddNode("compute", RELU, 1, 1);
  auto output = builder.AddNode("output", NETOUTPUT, 0, 0);
  enter->GetOpDesc()->SetOpEngineName("DNN_VM_RTS");
  builder.AddDataEdge(data, 0, enter, 0);
  builder.AddDataEdge(enter, 0, compute, 0);
  builder.AddControlEdge(compute, output);
  auto graph = builder.GetGraph();

  ASSERT_EQ(PrunePass(PrunePass::Stage::kAfterEngineAssigned).Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), 3U);
  EXPECT_EQ(graph->FindNode("compute"), nullptr);
  EXPECT_EQ(graph->FindNode("enter"), enter);
  EXPECT_TRUE(data->GetOutDataAnchor(0)->IsLinkedWith(enter->GetInDataAnchor(0)));
  EXPECT_TRUE(enter->GetOutControlAnchor()->IsLinkedWith(output->GetInControlAnchor()));
  EXPECT_EQ(output->GetInControlAnchor()->GetPeerAnchorsSize(), 1U);
}

// 不依赖出口连边或执行标签，传统控制流节点及其输入链仍由分配后的引擎保活。
TEST_F(UtestGraphPassesPrunePass, after_engine_prune_keeps_legacy_control_flow_by_assigned_engine) {
  const std::vector<std::pair<std::string, std::vector<std::string>>> engine_types = {
      {"DNN_VM_RTS", {ENTER, REFENTER, EXIT, REFEXIT, NEXTITERATION, REFNEXTITERATION, LOOPCOND}},
      {"DNN_VM_RTS_FFTS_PLUS", {ENTER, REFENTER, EXIT, REFEXIT, NEXTITERATION, REFNEXTITERATION, LOOPCOND}},
      {"DNN_VM_GE_LOCAL", {SWITCH, REFSWITCH, MERGE, REFMERGE, CONTROLTRIGGER}}};
  for (const auto &engine : engine_types) {
    for (const auto &type : engine.second) {
      SCOPED_TRACE(engine.first + ":" + type);
      ut::GraphBuilder builder("legacy_engine_keep_alive");
      auto data = builder.AddNode("data", DATA, 0, 1);
      auto producer = builder.AddNode("producer", RELU, 1, 1);
      auto control = builder.AddNode("control", type, 1, 1);
      auto dead = builder.AddNode("dead", RELU, 1, 1);
      builder.AddNode("output", NETOUTPUT, 0, 0);
      control->GetOpDesc()->SetOpEngineName(engine.first);
      builder.AddDataEdge(data, 0, producer, 0);
      builder.AddDataEdge(producer, 0, control, 0);
      builder.AddDataEdge(data, 0, dead, 0);
      auto graph = builder.GetGraph();

      ASSERT_EQ(PrunePass(PrunePass::Stage::kAfterEngineAssigned).Run(graph), SUCCESS);
      EXPECT_EQ(graph->GetDirectNodesSize(), 4U);
      EXPECT_EQ(graph->FindNode("control"), control);
      EXPECT_EQ(graph->FindNode("producer"), producer);
      EXPECT_EQ(graph->FindNode("dead"), nullptr);
      EXPECT_TRUE(data->GetOutDataAnchor(0)->IsLinkedWith(producer->GetInDataAnchor(0)));
      EXPECT_TRUE(producer->GetOutDataAnchor(0)->IsLinkedWith(control->GetInDataAnchor(0)));
    }
  }
}

// SwitchN 按显式 target 保留，类型本身不再阻止自动删除下游冗余分支。
TEST_F(UtestGraphPassesPrunePass, after_engine_prune_restores_explicit_switch_n_target_completion) {
  ut::GraphBuilder builder("switch_n_target_completion");
  auto data = builder.AddNode("data", DATA, 0, 1);
  auto pred = builder.AddNode("pred", DATA, 0, 1);
  auto producer = builder.AddNode("producer", RELU, 1, 1);
  auto control = builder.AddNode("control", SWITCHN, 2, 2);
  auto compute = builder.AddNode("compute", RELU, 1, 1);
  auto output = builder.AddNode("output", NETOUTPUT, 0, 0);
  builder.AddDataEdge(data, 0, producer, 0);
  builder.AddDataEdge(producer, 0, control, 0);
  builder.AddDataEdge(pred, 0, control, 1);
  builder.AddDataEdge(control, 0, compute, 0);
  builder.AddControlEdge(compute, output);
  auto graph = builder.GetGraph();
  graph->SetGraphTargetNodesInfo({control});

  ASSERT_EQ(PrunePass(PrunePass::Stage::kAfterEngineAssigned).Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), 5U);
  EXPECT_EQ(graph->FindNode("control"), control);
  EXPECT_EQ(graph->FindNode("producer"), producer);
  EXPECT_EQ(graph->FindNode("compute"), nullptr);
  EXPECT_TRUE(data->GetOutDataAnchor(0)->IsLinkedWith(producer->GetInDataAnchor(0)));
  EXPECT_TRUE(producer->GetOutDataAnchor(0)->IsLinkedWith(control->GetInDataAnchor(0)));
  EXPECT_TRUE(pred->GetOutDataAnchor(0)->IsLinkedWith(control->GetInDataAnchor(1)));
  EXPECT_EQ(control->GetOutDataAnchor(0)->GetPeerAnchorsSize(), 0U);
  EXPECT_TRUE(control->GetOutControlAnchor()->IsLinkedWith(output->GetInControlAnchor()));
  EXPECT_EQ(output->GetInControlAnchor()->GetPeerAnchorsSize(), 1U);
}

TEST_F(UtestGraphPassesPrunePass, after_engine_prune_preserves_legacy_kernel_library_keep_alive) {
  ut::GraphBuilder builder("legacy_kernel_library");
  auto keep = builder.AddNode("keep", "KeptByKernelLibrary", 0, 1);
  auto output = builder.AddNode("output", NETOUTPUT, 0, 0);
  keep->GetOpDesc()->SetOpKernelLibName(kEngineNameGeLocal);
  builder.AddControlEdge(keep, output);
  auto graph = builder.GetGraph();

  ASSERT_EQ(PrunePass(PrunePass::Stage::kAfterEngineAssigned).Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), 2U);
  EXPECT_TRUE(keep->GetOutControlAnchor()->IsLinkedWith(output->GetInControlAnchor()));
}

TEST_F(UtestGraphPassesPrunePass, after_engine_prune_only_ignores_ordinary_control_anchor_pair) {
  ut::GraphBuilder builder("parallel_mixed_completion");
  auto source = builder.AddNode("source", RELU, 0, 1);
  auto output = builder.AddNode("output", NETOUTPUT, 0, 0);
  builder.AddControlEdge(source, output);
  builder.AddControlEdge(source, output);
  auto graph = builder.GetGraph();
  ASSERT_EQ(GraphUtils::AddEdge(source->GetOutDataAnchor(0), output->GetInControlAnchor()), GRAPH_SUCCESS);

  ASSERT_EQ(PrunePass(PrunePass::Stage::kAfterEngineAssigned).Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), 2U);
  EXPECT_TRUE(source->GetOutDataAnchor(0)->IsLinkedWith(output->GetInControlAnchor()));
  EXPECT_FALSE(source->GetOutControlAnchor()->IsLinkedWith(output->GetInControlAnchor()));
}

TEST_F(UtestGraphPassesPrunePass, after_engine_prune_deduplicates_shared_boundary_for_one_output) {
  ut::GraphBuilder builder("shared_boundary");
  auto keep = builder.AddNode("keep", "KeptByEngine", 0, 1);
  auto dead1 = builder.AddNode("dead1", RELU, 1, 1);
  auto dead2 = builder.AddNode("dead2", RELU, 1, 1);
  auto output = builder.AddNode("output", NETOUTPUT, 0, 0);
  keep->GetOpDesc()->SetOpEngineName("DNN_VM_GE_LOCAL");
  builder.AddDataEdge(keep, 0, dead1, 0);
  builder.AddDataEdge(keep, 0, dead2, 0);
  builder.AddControlEdge(dead1, output);
  builder.AddControlEdge(dead2, output);
  auto graph = builder.GetGraph();

  ASSERT_EQ(PrunePass(PrunePass::Stage::kAfterEngineAssigned).Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), 2U);
  EXPECT_EQ(graph->FindNode("dead1"), nullptr);
  EXPECT_EQ(graph->FindNode("dead2"), nullptr);
  EXPECT_TRUE(keep->GetOutControlAnchor()->IsLinkedWith(output->GetInControlAnchor()));
  EXPECT_EQ(output->GetInControlAnchor()->GetPeerAnchorsSize(), 1U);
}

TEST_F(UtestGraphPassesPrunePass, after_engine_prune_preserves_completion_in_labeled_execution_frame) {
  for (const auto &attribute : {ATTR_NAME_STREAM_LABEL, ATTR_NAME_BATCH_LABEL}) {
    SCOPED_TRACE(attribute);
    ut::GraphBuilder builder("execution_label");
    auto data = builder.AddNode("data", DATA, 0, 1);
    auto compute = builder.AddNode("compute", RELU, 1, 1);
    auto output = builder.AddNode("output", NETOUTPUT, 0, 0);
    builder.AddDataEdge(data, 0, compute, 0);
    builder.AddControlEdge(compute, output);
    auto graph = builder.GetGraph();
    ASSERT_TRUE(AttrUtils::SetStr(compute->GetOpDesc(), attribute, "active_branch"));

    ASSERT_EQ(PrunePass(PrunePass::Stage::kAfterEngineAssigned).Run(graph), SUCCESS);
    EXPECT_EQ(graph->GetDirectNodesSize(), 3U);
    EXPECT_TRUE(compute->GetOutControlAnchor()->IsLinkedWith(output->GetInControlAnchor()));
  }
}

// 图中个别节点带执行标签时，无标签的候选分支照常剪枝；带标签节点仅按保活规则保留自身。
TEST_F(UtestGraphPassesPrunePass, after_engine_prune_still_prunes_unlabeled_candidates_in_labeled_graph) {
  ut::GraphBuilder builder("labeled_graph_prunes_candidate");
  auto data = builder.AddNode("data", DATA, 0, 1);
  auto labeled = builder.AddNode("labeled", RELU, 1, 1);
  auto compute = builder.AddNode("compute", RELU, 1, 1);
  auto output = builder.AddNode("output", NETOUTPUT, 0, 0);
  builder.AddDataEdge(data, 0, labeled, 0);
  builder.AddDataEdge(labeled, 0, compute, 0);
  builder.AddControlEdge(compute, output);
  auto graph = builder.GetGraph();
  ASSERT_TRUE(AttrUtils::SetStr(labeled->GetOpDesc(), ATTR_NAME_STREAM_LABEL, "active_stream"));

  ASSERT_EQ(PrunePass(PrunePass::Stage::kAfterEngineAssigned).Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), 3U);
  EXPECT_EQ(graph->FindNode("compute"), nullptr);
  EXPECT_EQ(graph->FindNode("labeled"), labeled);
  EXPECT_TRUE(labeled->GetOutControlAnchor()->IsLinkedWith(output->GetInControlAnchor()));
  EXPECT_EQ(output->GetInControlAnchor()->GetPeerAnchorsSize(), 1U);
}

// 历史候选属性无论类型或内容如何，都不影响自动剪枝。
TEST_F(UtestGraphPassesPrunePass, after_engine_prune_does_not_depend_on_legacy_candidate_attribute) {
  for (const bool invalid_type : {false, true}) {
    SCOPED_TRACE(invalid_type);
    ut::GraphBuilder builder("legacy_permission");
    auto source = builder.AddNode("source", RELU, 0, 1);
    auto output = builder.AddNode("output", NETOUTPUT, 0, 0);
    builder.AddControlEdge(source, output);
    auto graph = builder.GetGraph();
    if (invalid_type) {
      ASSERT_TRUE(AttrUtils::SetBool(output->GetOpDesc(), "_prunable_control_inputs", true));
    } else {
      ASSERT_TRUE(AttrUtils::SetListStr(output->GetOpDesc(), "_prunable_control_inputs", {"expired_source"}));
    }

    ASSERT_EQ(PrunePass(PrunePass::Stage::kAfterEngineAssigned).Run(graph), SUCCESS);
    EXPECT_EQ(graph->GetDirectNodesSize(), 1U);
    EXPECT_EQ(graph->FindNode("source"), nullptr);
    EXPECT_EQ(output->GetInControlAnchor()->GetPeerAnchorsSize(), 0U);
  }
}

// 算子类型不在白名单中时，分别验证引擎名和算子库名能够独立保活。
TEST_F(UtestGraphPassesPrunePass, after_engine_prune_keeps_engine_and_kernel_identities_independently) {
  const std::vector<std::pair<std::string, std::string>> identities = {{"DNN_HCCL", "unlisted_kernel"},
                                                                       {"DNN_VM_RTS_FFTS_PLUS", "unlisted_kernel"},
                                                                       {"unlisted_engine", kEngineNameHccl},
                                                                       {"unlisted_engine", kEngineNameRts}};
  for (const auto &identity : identities) {
    SCOPED_TRACE(identity.first + ":" + identity.second);
    ut::GraphBuilder builder("keep_alive_identity");
    auto data = builder.AddNode("data", DATA, 0, 1);
    auto producer = builder.AddNode("producer", RELU, 1, 1);
    auto keep = builder.AddNode("keep", "OpaqueEngineOp", 1, 1);
    auto output = builder.AddNode("output", NETOUTPUT, 0, 0);
    keep->GetOpDesc()->SetOpEngineName(identity.first);
    keep->GetOpDesc()->SetOpKernelLibName(identity.second);
    builder.AddDataEdge(data, 0, producer, 0);
    builder.AddDataEdge(producer, 0, keep, 0);
    builder.AddControlEdge(keep, output);
    auto graph = builder.GetGraph();

    ASSERT_EQ(PrunePass(PrunePass::Stage::kAfterEngineAssigned).Run(graph), SUCCESS);
    EXPECT_EQ(graph->GetDirectNodesSize(), 4U);
    EXPECT_EQ(graph->FindNode("keep"), keep);
    EXPECT_EQ(graph->FindNode("producer"), producer);
    EXPECT_TRUE(data->GetOutDataAnchor(0)->IsLinkedWith(producer->GetInDataAnchor(0)));
    EXPECT_TRUE(producer->GetOutDataAnchor(0)->IsLinkedWith(keep->GetInDataAnchor(0)));
    EXPECT_TRUE(keep->GetOutControlAnchor()->IsLinkedWith(output->GetInControlAnchor()));
  }
}

// Horovod 在 GE 和图元数据中存在两组明确类型名，不依赖字符串前缀判断通信语义。
TEST_F(UtestGraphPassesPrunePass, after_engine_prune_preserves_horovod_types_without_engine_match) {
  const std::vector<std::string> types = {
      "HorovodAllreduce",     "HorovodAllgather",     "HorovodBroadcast",     "HorovodWait",
      "hvdCallbackAllreduce", "hvdCallbackAllgather", "hvdCallbackBroadcast", "hvdWait"};
  for (const auto &type : types) {
    SCOPED_TRACE(type);
    ut::GraphBuilder builder("horovod_type");
    auto data = builder.AddNode("data", DATA, 0, 1);
    auto producer = builder.AddNode("producer", RELU, 1, 1);
    auto communication = builder.AddNode("communication", type, 1, 1);
    auto output = builder.AddNode("output", NETOUTPUT, 0, 0);
    communication->GetOpDesc()->SetOpEngineName("unlisted_engine");
    communication->GetOpDesc()->SetOpKernelLibName("unlisted_kernel");
    builder.AddDataEdge(data, 0, producer, 0);
    builder.AddDataEdge(producer, 0, communication, 0);
    builder.AddControlEdge(communication, output);
    auto graph = builder.GetGraph();

    ASSERT_EQ(PrunePass(PrunePass::Stage::kAfterEngineAssigned).Run(graph), SUCCESS);
    EXPECT_EQ(graph->GetDirectNodesSize(), 4U);
    EXPECT_EQ(graph->FindNode("communication"), communication);
    EXPECT_EQ(graph->FindNode("producer"), producer);
    EXPECT_TRUE(producer->GetOutDataAnchor(0)->IsLinkedWith(communication->GetInDataAnchor(0)));
    EXPECT_TRUE(communication->GetOutControlAnchor()->IsLinkedWith(output->GetInControlAnchor()));
  }
}

// 名称相似不能证明通信语义，带通信前缀但不在明确类型表中的普通死节点仍可裁剪。
TEST_F(UtestGraphPassesPrunePass, after_engine_prune_does_not_keep_nodes_by_communication_prefix) {
  for (const auto &type : {"HcomPureOp", "hvdPureOp", "HorovodPureOp", "Hco", "hv", "hcomAllReduce", "HvdAllreduce",
                           "CustomHcomAllReduce", "CustomhvdAllreduce"}) {
    SCOPED_TRACE(type);
    ut::GraphBuilder builder("non_communication_prefix");
    auto data = builder.AddNode("data", DATA, 0, 1);
    auto dead = builder.AddNode("dead", type, 1, 1);
    auto output = builder.AddNode("output", NETOUTPUT, 0, 0);
    builder.AddDataEdge(data, 0, dead, 0);
    builder.AddControlEdge(dead, output);
    auto graph = builder.GetGraph();

    ASSERT_EQ(PrunePass(PrunePass::Stage::kAfterEngineAssigned).Run(graph), SUCCESS);
    EXPECT_EQ(graph->GetDirectNodesSize(), 2U);
    EXPECT_EQ(graph->FindNode("dead"), nullptr);
    EXPECT_EQ(data->GetOutDataAnchor(0)->GetPeerAnchorsSize(), 0U);
    EXPECT_TRUE(data->GetOutControlAnchor()->IsLinkedWith(output->GetInControlAnchor()));
    EXPECT_EQ(output->GetInControlAnchor()->GetPeerAnchorsSize(), 1U);
  }
}

// 数据获取会推进队列或迭代状态，即使张量结果无消费者也保留其完成依赖。
TEST_F(UtestGraphPassesPrunePass, after_engine_prune_preserves_getnext_variants) {
  for (const auto &type : {GETNEXT, DYNAMICGETNEXT, DYNAMICGETNEXTV2}) {
    SCOPED_TRACE(type);
    ut::GraphBuilder builder("getnext_variants");
    auto getnext = builder.AddNode("getnext", type, 0, 1);
    auto dead = builder.AddNode("dead", RELU, 1, 1);
    auto output = builder.AddNode("output", NETOUTPUT, 0, 0);
    getnext->GetOpDesc()->SetOpEngineName("DNN_VM_AICPU");
    builder.AddDataEdge(getnext, 0, dead, 0);
    builder.AddControlEdge(getnext, output);
    auto graph = builder.GetGraph();

    ASSERT_EQ(PrunePass(PrunePass::Stage::kAfterEngineAssigned).Run(graph), SUCCESS);
    EXPECT_EQ(graph->GetDirectNodesSize(), 2U);
    EXPECT_EQ(graph->FindNode("getnext"), getnext);
    EXPECT_EQ(graph->FindNode("dead"), nullptr);
    EXPECT_EQ(getnext->GetOutDataAnchor(0)->GetPeerAnchorsSize(), 0U);
    EXPECT_TRUE(getnext->GetOutControlAnchor()->IsLinkedWith(output->GetInControlAnchor()));
  }
}

// PrintV3 在 TF 引擎映射为 Print，输出未被使用也不能删除打印及其输入计算。
TEST_F(UtestGraphPassesPrunePass, after_engine_prune_preserves_print_v3_input_closure) {
  ut::GraphBuilder builder("print_v3_input_closure");
  auto data = builder.AddNode("data", DATA, 0, 1);
  auto producer = builder.AddNode("producer", RELU, 1, 1);
  auto print = builder.AddNode("print", "PrintV3", 1, 1);
  auto output = builder.AddNode("output", NETOUTPUT, 0, 0);
  print->GetOpDesc()->SetOpEngineName("DNN_VM_AICPU");
  builder.AddDataEdge(data, 0, producer, 0);
  builder.AddDataEdge(producer, 0, print, 0);
  builder.AddControlEdge(print, output);
  auto graph = builder.GetGraph();

  ASSERT_EQ(PrunePass(PrunePass::Stage::kAfterEngineAssigned).Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), 4U);
  EXPECT_EQ(graph->FindNode("print"), print);
  EXPECT_EQ(graph->FindNode("producer"), producer);
  EXPECT_TRUE(data->GetOutDataAnchor(0)->IsLinkedWith(producer->GetInDataAnchor(0)));
  EXPECT_TRUE(producer->GetOutDataAnchor(0)->IsLinkedWith(print->GetInDataAnchor(0)));
  EXPECT_TRUE(print->GetOutControlAnchor()->IsLinkedWith(output->GetInControlAnchor()));
}

// 边界节点虽非白名单，但被另一出口需要，删除死链时仍须恢复它到原出口的完成依赖。
TEST_F(UtestGraphPassesPrunePass, after_engine_prune_restores_non_whitelist_boundary_to_original_output) {
  ut::GraphBuilder builder("live_boundary_across_outputs");
  auto data = builder.AddNode("data", DATA, 0, 1);
  auto shared = builder.AddNode("shared", RELU, 1, 1);
  auto dead = builder.AddNode("dead", RELU, 1, 1);
  auto output1 = builder.AddNode("output1", NETOUTPUT, 0, 0);
  auto output2 = builder.AddNode("output2", NETOUTPUT, 1, 0);
  builder.AddDataEdge(data, 0, shared, 0);
  builder.AddDataEdge(shared, 0, dead, 0);
  builder.AddDataEdge(shared, 0, output2, 0);
  builder.AddControlEdge(dead, output1);
  auto graph = builder.GetGraph();

  ASSERT_EQ(PrunePass(PrunePass::Stage::kAfterEngineAssigned).Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), 4U);
  EXPECT_EQ(graph->FindNode("dead"), nullptr);
  EXPECT_TRUE(data->GetOutDataAnchor(0)->IsLinkedWith(shared->GetInDataAnchor(0)));
  EXPECT_TRUE(shared->GetOutDataAnchor(0)->IsLinkedWith(output2->GetInDataAnchor(0)));
  EXPECT_TRUE(shared->GetOutControlAnchor()->IsLinkedWith(output1->GetInControlAnchor()));
  EXPECT_FALSE(shared->GetOutControlAnchor()->IsLinkedWith(output2->GetInControlAnchor()));
  EXPECT_FALSE(data->GetOutControlAnchor()->IsLinkedWith(output1->GetInControlAnchor()));
}

// 全图 V-H 清理也包含候选控制边上游之外的独立死链，不将原始叶子自动当作保留根。
TEST_F(UtestGraphPassesPrunePass, after_engine_prune_removes_dead_branch_outside_candidate_source_ancestors) {
  ut::GraphBuilder builder("global_dead_nodes");
  auto data = builder.AddNode("data", DATA, 0, 1);
  auto live = builder.AddNode("live", RELU, 1, 1);
  auto marked = builder.AddNode("marked", RELU, 1, 1);
  auto orphan = builder.AddNode("orphan", RELU, 1, 1);
  auto leaf = builder.AddNode("leaf", RELU, 1, 1);
  auto output = builder.AddNode("output", NETOUTPUT, 1, 0);
  builder.AddDataEdge(data, 0, live, 0);
  builder.AddDataEdge(live, 0, output, 0);
  builder.AddDataEdge(live, 0, marked, 0);
  builder.AddControlEdge(marked, output);
  builder.AddDataEdge(data, 0, orphan, 0);
  builder.AddDataEdge(orphan, 0, leaf, 0);
  auto graph = builder.GetGraph();

  ASSERT_EQ(PrunePass(PrunePass::Stage::kAfterEngineAssigned).Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), 3U);
  for (const auto &name : {"marked", "orphan", "leaf"}) {
    EXPECT_EQ(graph->FindNode(name), nullptr);
  }
  EXPECT_TRUE(data->GetOutDataAnchor(0)->IsLinkedWith(live->GetInDataAnchor(0)));
  EXPECT_TRUE(live->GetOutDataAnchor(0)->IsLinkedWith(output->GetInDataAnchor(0)));
  EXPECT_EQ(output->GetInControlAnchor()->GetPeerAnchorsSize(), 0U);
}

// 混合锚点边必须继续参与输出的保留闭包，不作为自动剪枝候选。
TEST_F(UtestGraphPassesPrunePass, after_engine_prune_preserves_mixed_output_dependencies) {
  for (const bool data_to_control : {false, true}) {
    SCOPED_TRACE(data_to_control);
    ut::GraphBuilder builder("marked_mixed_output");
    auto data = builder.AddNode("data", DATA, 0, 1);
    auto producer = builder.AddNode("producer", RELU, 1, 1);
    auto output = builder.AddNode("output", NETOUTPUT, data_to_control ? 0 : 1, 0);
    builder.AddDataEdge(data, 0, producer, 0);
    auto graph = builder.GetGraph();
    const AnchorPtr source =
        data_to_control ? AnchorPtr(producer->GetOutDataAnchor(0)) : AnchorPtr(producer->GetOutControlAnchor());
    const AnchorPtr target =
        data_to_control ? AnchorPtr(output->GetInControlAnchor()) : AnchorPtr(output->GetInDataAnchor(0));
    if (data_to_control) {
      ASSERT_EQ(GraphUtils::AddEdge(producer->GetOutDataAnchor(0), output->GetInControlAnchor()), GRAPH_SUCCESS);
    } else {
      ASSERT_EQ(GraphUtils::AddEdge(producer->GetOutControlAnchor(), output->GetInDataAnchor(0)), GRAPH_SUCCESS);
    }

    ASSERT_EQ(PrunePass(PrunePass::Stage::kAfterEngineAssigned).Run(graph), SUCCESS);
    EXPECT_EQ(graph->GetDirectNodesSize(), 3U);
    EXPECT_EQ(graph->FindNode("producer"), producer);
    EXPECT_TRUE(source->IsLinkedWith(target));
    EXPECT_TRUE(data->GetOutDataAnchor(0)->IsLinkedWith(producer->GetInDataAnchor(0)));
  }
}

// 已有模型数据输出时，只移除同源的冗余控制边，不改变输出端口或生产者。
TEST_F(UtestGraphPassesPrunePass, after_engine_prune_preserves_data_output_with_parallel_control) {
  ut::GraphBuilder builder("parallel_data_output");
  auto data = builder.AddNode("data", DATA, 0, 1);
  auto producer = builder.AddNode("producer", RELU, 1, 1);
  auto output = builder.AddNode("output", NETOUTPUT, 1, 0);
  builder.AddDataEdge(data, 0, producer, 0);
  builder.AddDataEdge(producer, 0, output, 0);
  builder.AddControlEdge(producer, output);
  auto graph = builder.GetGraph();

  ASSERT_EQ(PrunePass(PrunePass::Stage::kAfterEngineAssigned).Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), 3U);
  EXPECT_TRUE(producer->GetOutDataAnchor(0)->IsLinkedWith(output->GetInDataAnchor(0)));
  EXPECT_TRUE(data->GetOutDataAnchor(0)->IsLinkedWith(producer->GetInDataAnchor(0)));
  EXPECT_EQ(output->GetAllInDataAnchorsSize(), 1U);
  EXPECT_EQ(output->GetInControlAnchor()->GetPeerAnchorsSize(), 0U);
}

// 子图单独计算 H 和边界补边，子图 Data 的完成依赖不得错误连到父图出口。
TEST_F(UtestGraphPassesPrunePass, after_engine_prune_keeps_subgraph_replacements_in_their_owner_graph) {
  ut::GraphBuilder builder("parent_with_prunable_body");
  auto call = builder.AddNode("call", "CustomCall", 0, 1);
  auto output = builder.AddNode("output", NETOUTPUT, 0, 0);
  builder.AddControlEdge(call, output);
  auto graph = builder.GetGraph();
  ut::GraphBuilder sub_builder("prunable_body");
  auto inner_data = sub_builder.AddNode("inner_data", DATA, 0, 1);
  auto dead = sub_builder.AddNode("dead", RELU, 1, 1);
  auto inner_output = sub_builder.AddNode("inner_output", NETOUTPUT, 0, 0);
  sub_builder.AddDataEdge(inner_data, 0, dead, 0);
  sub_builder.AddControlEdge(dead, inner_output);
  auto subgraph = sub_builder.GetGraph();
  ut::GraphBuilder::AddPartitionedCall(graph, "call", subgraph);
  PrunePass pass(PrunePass::Stage::kAfterEngineAssigned);
  std::vector<std::pair<string, GraphPass *>> passes = {{"PrunePass", &pass}};

  ASSERT_EQ(PassManager::Run(graph, passes), SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), 2U);
  EXPECT_EQ(subgraph->GetDirectNodesSize(), 2U);
  EXPECT_EQ(subgraph->FindNode("dead"), nullptr);
  EXPECT_TRUE(call->GetOutControlAnchor()->IsLinkedWith(output->GetInControlAnchor()));
  EXPECT_TRUE(inner_data->GetOutControlAnchor()->IsLinkedWith(inner_output->GetInControlAnchor()));
  EXPECT_FALSE(inner_data->GetOutControlAnchor()->IsLinkedWith(output->GetInControlAnchor()));
  EXPECT_EQ(inner_data->GetOwnerComputeGraph(), subgraph);
}

// NetOutputPass 自动补上的叶子完成边无需额外打标，即可触发冗余链剪枝。
TEST_F(UtestGraphPassesPrunePass, after_engine_prune_removes_leaf_completion_added_by_netoutput_pass) {
  ut::GraphBuilder builder("automatic_leaf_completion");
  auto data = builder.AddNode("data", DATA, 0, 1);
  auto live = builder.AddNode("live", RELU, 1, 1);
  auto dead1 = builder.AddNode("dead1", RELU, 1, 1);
  auto dead2 = builder.AddNode("dead2", RELU, 1, 1);
  builder.AddDataEdge(data, 0, live, 0);
  builder.AddDataEdge(live, 0, dead1, 0);
  builder.AddDataEdge(dead1, 0, dead2, 0);
  auto graph = builder.GetGraph();
  graph->SetGraphOutNodesInfo({{live, 0}});

  ASSERT_EQ(NetOutputPass().Run(graph), SUCCESS);
  const auto output = graph->FindFirstNodeMatchType(NETOUTPUT);
  ASSERT_NE(output, nullptr);
  ASSERT_TRUE(dead2->GetOutControlAnchor()->IsLinkedWith(output->GetInControlAnchor()));

  ASSERT_EQ(PrunePass(PrunePass::Stage::kAfterEngineAssigned).Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), 3U);
  EXPECT_EQ(graph->FindNode("dead1"), nullptr);
  EXPECT_EQ(graph->FindNode("dead2"), nullptr);
  EXPECT_TRUE(data->GetOutDataAnchor(0)->IsLinkedWith(live->GetInDataAnchor(0)));
  EXPECT_TRUE(live->GetOutDataAnchor(0)->IsLinkedWith(output->GetInDataAnchor(0)));
  EXPECT_EQ(output->GetInControlAnchor()->GetPeerAnchorsSize(), 0U);
}
