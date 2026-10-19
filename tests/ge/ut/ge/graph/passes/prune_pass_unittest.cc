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
#include "graph/passes/standard_optimize/prune_pass.h"

#include "anchor.h"
#include "common/debug/log.h"
#include "common/debug/memory_dumper.h"
#include "common/framework_types_internal.h"
#include "framework/common/ge_inner_error_codes.h"
#include "framework/common/ge_types.h"
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
  ge::OpDescUtilsEx::SetType(reverse_op1, REVERSE);
  reverse_op1->SetName("Reverse1");
  reverse_op1->AddOutputDesc(ge::GeTensorDesc());
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

// inner NetOutput（GE自建，用户未指定输出）：NetOutputPass为保活/保序给所有叶子一刀切补了
// 控制边。PrunePass扩展：沿inner NetOutput控制入边识别空闲节点（纯计算死叶子），剪除其
// 误连的保活边后死链变回不可达，由既有BFS+处置逻辑整链剪除；保活节点（Assign）的控制边
// 保留，其自身及输入经该边天然可达，无需救援
TEST_F(UtestGraphPassesPrunePass, inner_netoutput_prune_dead_chain_by_remove_idle_ctrl_edge) {
  auto graph_builder = ut::GraphBuilder("inner");
  auto data_1 = graph_builder.AddNode("data_1", DATA, 0, 1);
  auto floor = graph_builder.AddNode("floor", FLOOR, 1, 1);
  auto dead_1 = graph_builder.AddNode("dead_1", REVERSE, 1, 1);
  auto dead_2 = graph_builder.AddNode("dead_2", FLOOR, 1, 1);
  auto var_1 = graph_builder.AddNode("var_1", VARIABLE, 0, 1);
  auto assign_1 = graph_builder.AddNode("assign_1", ASSIGN, 2, 1);
  auto out_0 = graph_builder.AddNode("out", NETOUTPUT, 1, 1);
  graph_builder.AddDataEdge(data_1, 0, floor, 0);
  graph_builder.AddDataEdge(floor, 0, out_0, 0);
  graph_builder.AddDataEdge(data_1, 0, dead_1, 0);
  graph_builder.AddDataEdge(dead_1, 0, dead_2, 0);
  graph_builder.AddDataEdge(var_1, 0, assign_1, 0);
  graph_builder.AddDataEdge(data_1, 0, assign_1, 1);
  AttrUtils::SetBool(out_0->GetOpDesc(), ATTR_NAME_INNER_NET_OUTPUT, true);
  // 模拟NetOutputPass全连叶子后的真实图状态：普通死叶子dead_2与副作用叶子assign_1都带控制边
  EXPECT_EQ(GraphUtils::AddEdge(dead_2->GetOutControlAnchor(), out_0->GetInControlAnchor()), GRAPH_SUCCESS);
  EXPECT_EQ(GraphUtils::AddEdge(assign_1->GetOutControlAnchor(), out_0->GetInControlAnchor()), GRAPH_SUCCESS);

  auto graph = graph_builder.GetGraph();
  PrunePass prune_pass;
  std::vector<std::pair<string, GraphPass *>> passes = {{"PrunePass", &prune_pass}};
  Status status = PassManager::Run(graph, passes);
  EXPECT_EQ(ge::SUCCESS, status);

  // 空闲节点dead_2的误连控制边被剪除，死链dead_1/dead_2整链不可达被删
  EXPECT_EQ(graph->FindNode("dead_1"), nullptr);
  EXPECT_EQ(graph->FindNode("dead_2"), nullptr);
  // Assign的控制边承载保活语义不删除，其自身经该边可达，输入var_1随之可达
  auto assign_res = graph->FindNode("assign_1");
  ASSERT_NE(assign_res, nullptr);
  EXPECT_TRUE(assign_res->GetOutControlAnchor()->IsLinkedWith(out_0->GetInControlAnchor()));
  EXPECT_NE(graph->FindNode("var_1"), nullptr);
}

// inner NetOutput图中，副作用节点（Assign）的输出消费者是死代码：死链叶子（NetOutputPass
// 所连控制边）被剪除后Assign不可达，须救援保活并补控制边；其输入Variable同样保活补边
TEST_F(UtestGraphPassesPrunePass, inner_netoutput_keep_alive_assign_with_dead_output) {
  auto graph_builder = ut::GraphBuilder("inner_assign");
  auto data_1 = graph_builder.AddNode("data_1", DATA, 0, 1);
  auto floor = graph_builder.AddNode("floor", FLOOR, 1, 1);
  auto var_1 = graph_builder.AddNode("var_1", VARIABLE, 0, 1);
  auto assign_1 = graph_builder.AddNode("assign_1", ASSIGN, 2, 1);
  auto dead_out = graph_builder.AddNode("dead_out", FLOOR, 1, 1);
  auto out_0 = graph_builder.AddNode("out", NETOUTPUT, 1, 1);
  graph_builder.AddDataEdge(data_1, 0, floor, 0);
  graph_builder.AddDataEdge(floor, 0, out_0, 0);
  graph_builder.AddDataEdge(var_1, 0, assign_1, 0);
  graph_builder.AddDataEdge(data_1, 0, assign_1, 1);
  graph_builder.AddDataEdge(assign_1, 0, dead_out, 0);
  AttrUtils::SetBool(out_0->GetOpDesc(), ATTR_NAME_INNER_NET_OUTPUT, true);
  // 死链叶子dead_out被NetOutputPass连了控制边（assign_1非叶子不连）
  EXPECT_EQ(GraphUtils::AddEdge(dead_out->GetOutControlAnchor(), out_0->GetInControlAnchor()), GRAPH_SUCCESS);

  auto graph = graph_builder.GetGraph();
  PrunePass prune_pass;
  std::vector<std::pair<string, GraphPass *>> passes = {{"PrunePass", &prune_pass}};
  Status status = PassManager::Run(graph, passes);
  EXPECT_EQ(ge::SUCCESS, status);

  // Assign的输出死链（含其误连控制边）被剪掉，Assign/var_1救援保活并补控制边保证执行
  EXPECT_EQ(graph->FindNode("dead_out"), nullptr);
  auto assign_res = graph->FindNode("assign_1");
  EXPECT_NE(assign_res, nullptr);
  auto var_res = graph->FindNode("var_1");
  EXPECT_NE(var_res, nullptr);
  const auto control_nodes = out_0->GetInControlNodes();
  EXPECT_EQ(control_nodes.size(), 2U);
  for (const auto &node : control_nodes) {
    const bool is_state_node = (node->GetName() == "assign_1") || (node->GetName() == "var_1");
    EXPECT_TRUE(is_state_node);
  }
}

// inner NetOutput图中，保活叶子（Assign）的控制边保留时，其Const/Variable输入经该边
// 天然可达一并保留，输入闭包保活不介入、不补多余控制边；无关死链仍被剪除
TEST_F(UtestGraphPassesPrunePass, inner_netoutput_keep_alive_leaf_ctrl_edge_preserved) {
  auto graph_builder = ut::GraphBuilder("inner_leaf");
  auto data_1 = graph_builder.AddNode("data_1", DATA, 0, 1);
  auto floor = graph_builder.AddNode("floor", FLOOR, 1, 1);
  auto dead_1 = graph_builder.AddNode("dead_1", REVERSE, 1, 1);
  auto var_1 = graph_builder.AddNode("var_1", VARIABLE, 0, 1);
  auto const_1 = graph_builder.AddNode("const_1", CONSTANT, 0, 1);
  auto assign_1 = graph_builder.AddNode("assign_1", ASSIGN, 2, 1);
  auto out_0 = graph_builder.AddNode("out", NETOUTPUT, 1, 1);
  graph_builder.AddDataEdge(data_1, 0, floor, 0);
  graph_builder.AddDataEdge(floor, 0, out_0, 0);
  graph_builder.AddDataEdge(data_1, 0, dead_1, 0);
  graph_builder.AddDataEdge(var_1, 0, assign_1, 0);
  graph_builder.AddDataEdge(const_1, 0, assign_1, 1);
  AttrUtils::SetBool(out_0->GetOpDesc(), ATTR_NAME_INNER_NET_OUTPUT, true);
  // 模拟NetOutputPass全连叶子：死叶子dead_1与副作用叶子assign_1都带控制边
  EXPECT_EQ(GraphUtils::AddEdge(dead_1->GetOutControlAnchor(), out_0->GetInControlAnchor()), GRAPH_SUCCESS);
  EXPECT_EQ(GraphUtils::AddEdge(assign_1->GetOutControlAnchor(), out_0->GetInControlAnchor()), GRAPH_SUCCESS);

  auto graph = graph_builder.GetGraph();
  PrunePass prune_pass;
  std::vector<std::pair<string, GraphPass *>> passes = {{"PrunePass", &prune_pass}};
  Status status = PassManager::Run(graph, passes);
  EXPECT_EQ(ge::SUCCESS, status);

  // 无关死链仍被剪除
  EXPECT_EQ(graph->FindNode("dead_1"), nullptr);
  // Assign控制边保留、天然可达，其输入var_1/const_1随之可达保留，输入不悬空且无多余补边
  auto assign_res = graph->FindNode("assign_1");
  ASSERT_NE(assign_res, nullptr);
  EXPECT_EQ(assign_res->GetInDataNodes().size(), 2U);
  EXPECT_NE(graph->FindNode("var_1"), nullptr);
  EXPECT_NE(graph->FindNode("const_1"), nullptr);
  EXPECT_FALSE(const_1->GetOutControlAnchor()->IsLinkedWith(out_0->GetInControlAnchor()));
}

// inner NetOutput图中，保活节点（如NetOutputPass之后才产生的悬挂Assign）没有到NetOutput的
// 控制边而不可达时：救援保活补控制边，其输入闭包（Const等非保活前驱）一并保活不补边，
// 避免Assign输入悬空（AssignRemovePass报peer anchor is nullptr）
TEST_F(UtestGraphPassesPrunePass, inner_netoutput_keep_alive_input_closure_without_ctrl_edge) {
  auto graph_builder = ut::GraphBuilder("inner_closure");
  auto data_1 = graph_builder.AddNode("data_1", DATA, 0, 1);
  auto floor = graph_builder.AddNode("floor", FLOOR, 1, 1);
  auto var_1 = graph_builder.AddNode("var_1", VARIABLE, 0, 1);
  auto const_1 = graph_builder.AddNode("const_1", CONSTANT, 0, 1);
  auto assign_1 = graph_builder.AddNode("assign_1", ASSIGN, 2, 1);
  auto dead_1 = graph_builder.AddNode("dead_1", REVERSE, 1, 1);
  auto out_0 = graph_builder.AddNode("out", NETOUTPUT, 1, 1);
  graph_builder.AddDataEdge(data_1, 0, floor, 0);
  graph_builder.AddDataEdge(floor, 0, out_0, 0);
  graph_builder.AddDataEdge(data_1, 0, dead_1, 0);
  graph_builder.AddDataEdge(var_1, 0, assign_1, 0);
  graph_builder.AddDataEdge(const_1, 0, assign_1, 1);
  AttrUtils::SetBool(out_0->GetOpDesc(), ATTR_NAME_INNER_NET_OUTPUT, true);
  // 死叶子dead_1带NetOutputPass控制边；assign_1悬挂无控制边（模拟迟到产生的保活叶子）
  EXPECT_EQ(GraphUtils::AddEdge(dead_1->GetOutControlAnchor(), out_0->GetInControlAnchor()), GRAPH_SUCCESS);

  auto graph = graph_builder.GetGraph();
  PrunePass prune_pass;
  std::vector<std::pair<string, GraphPass *>> passes = {{"PrunePass", &prune_pass}};
  Status status = PassManager::Run(graph, passes);
  EXPECT_EQ(ge::SUCCESS, status);

  // 无关死链仍被剪除
  EXPECT_EQ(graph->FindNode("dead_1"), nullptr);
  // 保活节点Assign救援保活，输入闭包（var_1/const_1）一并保留，输入不悬空
  auto assign_res = graph->FindNode("assign_1");
  ASSERT_NE(assign_res, nullptr);
  EXPECT_EQ(assign_res->GetInDataNodes().size(), 2U);
  EXPECT_NE(graph->FindNode("var_1"), nullptr);
  EXPECT_NE(graph->FindNode("const_1"), nullptr);
  // 保活节点补控制边保序；闭包节点const_1不补控制边
  for (const std::string &name : {"assign_1", "var_1"}) {
    bool found = false;
    for (const auto &node : out_0->GetInControlNodes()) {
      found = found || (node->GetName() == name);
    }
    EXPECT_TRUE(found);
  }
  EXPECT_FALSE(const_1->GetOutControlAnchor()->IsLinkedWith(out_0->GetInControlAnchor()));
}

// 非inner NetOutput（用户自建）与inner图统一剪枝语义：空闲节点的控制边按保活误连剪除，
// 死链重新不可达被删；不可达的保活节点（变量/Assign类）救援保活并补控制边，不区分图类型
TEST_F(UtestGraphPassesPrunePass, non_inner_netoutput_idle_ctrl_edge_removed_and_keep_alive_rescued) {
  auto graph_builder = ut::GraphBuilder("root");
  auto data_1 = graph_builder.AddNode("data_1", DATA, 0, 1);
  auto floor = graph_builder.AddNode("floor", FLOOR, 1, 1);
  auto var_1 = graph_builder.AddNode("var_1", VARIABLE, 0, 1);
  auto assign_1 = graph_builder.AddNode("assign_1", ASSIGN, 2, 1);
  auto dead_1 = graph_builder.AddNode("dead_1", REVERSE, 1, 1);
  auto out_0 = graph_builder.AddNode("out", NETOUTPUT, 1, 1);
  graph_builder.AddDataEdge(data_1, 0, floor, 0);
  graph_builder.AddDataEdge(floor, 0, out_0, 0);
  graph_builder.AddDataEdge(var_1, 0, assign_1, 0);
  graph_builder.AddDataEdge(data_1, 0, assign_1, 1);
  graph_builder.AddDataEdge(data_1, 0, dead_1, 0);
  // 用户自建NetOutput上空闲节点的控制边同为保活误连：剪除后死链重新不可达
  EXPECT_EQ(GraphUtils::AddEdge(dead_1->GetOutControlAnchor(), out_0->GetInControlAnchor()), GRAPH_SUCCESS);

  auto graph = graph_builder.GetGraph();
  PrunePass prune_pass;
  std::vector<std::pair<string, GraphPass *>> passes = {{"PrunePass", &prune_pass}};
  Status status = PassManager::Run(graph, passes);
  EXPECT_EQ(ge::SUCCESS, status);

  // 空闲控制边剪除，死链dead_1不可达被删；不可达的assign_1/var_1保活救援不区分inner
  EXPECT_EQ(graph->FindNode("dead_1"), nullptr);
  auto assign_res = graph->FindNode("assign_1");
  ASSERT_NE(assign_res, nullptr);
  EXPECT_TRUE(assign_res->GetOutControlAnchor()->IsLinkedWith(out_0->GetInControlAnchor()));
  EXPECT_NE(graph->FindNode("var_1"), nullptr);
}

// inner NetOutput图中，用户SetTargets指定的执行目标经控制边挂到NetOutput上
// （AddCtrlEdgeForTargets），表达真实执行意图：即使目标本身是纯计算节点，其控制边
// 也不能当空闲边剪除；目标及其输入保留，无关死链仍被剪除
TEST_F(UtestGraphPassesPrunePass, inner_netoutput_user_target_ctrl_edge_preserved) {
  auto graph_builder = ut::GraphBuilder("inner_target");
  auto data_1 = graph_builder.AddNode("data_1", DATA, 0, 1);
  auto floor = graph_builder.AddNode("floor", FLOOR, 1, 1);
  auto const_1 = graph_builder.AddNode("const_1", CONSTANT, 0, 1);
  auto target_1 = graph_builder.AddNode("target_1", FLOOR, 1, 1);
  auto dead_1 = graph_builder.AddNode("dead_1", REVERSE, 1, 1);
  auto out_0 = graph_builder.AddNode("out", NETOUTPUT, 1, 1);
  graph_builder.AddDataEdge(data_1, 0, floor, 0);
  graph_builder.AddDataEdge(floor, 0, out_0, 0);
  graph_builder.AddDataEdge(const_1, 0, target_1, 0);
  graph_builder.AddDataEdge(data_1, 0, dead_1, 0);
  AttrUtils::SetBool(out_0->GetOpDesc(), ATTR_NAME_INNER_NET_OUTPUT, true);
  // 模拟CreateOrUpdateNetoutput的AddCtrlEdgeForTargets，以及NetOutputPass给死叶子连的保活边
  EXPECT_EQ(GraphUtils::AddEdge(target_1->GetOutControlAnchor(), out_0->GetInControlAnchor()), GRAPH_SUCCESS);
  EXPECT_EQ(GraphUtils::AddEdge(dead_1->GetOutControlAnchor(), out_0->GetInControlAnchor()), GRAPH_SUCCESS);

  auto graph = graph_builder.GetGraph();
  std::vector<NodePtr> target_nodes = {target_1};
  EXPECT_EQ(graph->SetGraphTargetNodesInfo(target_nodes), GRAPH_SUCCESS);
  PrunePass prune_pass;
  std::vector<std::pair<string, GraphPass *>> passes = {{"PrunePass", &prune_pass}};
  Status status = PassManager::Run(graph, passes);
  EXPECT_EQ(ge::SUCCESS, status);

  // 用户target控制边保留，target及其输入闭包（const_1）保留；无关死链仍被剪除
  auto target_res = graph->FindNode("target_1");
  ASSERT_NE(target_res, nullptr);
  EXPECT_TRUE(target_res->GetOutControlAnchor()->IsLinkedWith(out_0->GetInControlAnchor()));
  EXPECT_NE(graph->FindNode("const_1"), nullptr);
  EXPECT_EQ(graph->FindNode("dead_1"), nullptr);
}

// inner NetOutput图中，同一空闲叶子到NetOutput存在多条平行控制边（LinkTo不查重）：
// 须循环删至删空，残留边会让BFS仍回溯到该节点，死链一条都剪不掉
TEST_F(UtestGraphPassesPrunePass, inner_netoutput_prune_idle_node_with_parallel_ctrl_edges) {
  auto graph_builder = ut::GraphBuilder("inner_parallel");
  auto data_1 = graph_builder.AddNode("data_1", DATA, 0, 1);
  auto floor = graph_builder.AddNode("floor", FLOOR, 1, 1);
  auto dead_1 = graph_builder.AddNode("dead_1", REVERSE, 1, 1);
  auto dead_2 = graph_builder.AddNode("dead_2", FLOOR, 1, 1);
  auto out_0 = graph_builder.AddNode("out", NETOUTPUT, 1, 1);
  graph_builder.AddDataEdge(data_1, 0, floor, 0);
  graph_builder.AddDataEdge(floor, 0, out_0, 0);
  graph_builder.AddDataEdge(data_1, 0, dead_1, 0);
  graph_builder.AddDataEdge(dead_1, 0, dead_2, 0);
  AttrUtils::SetBool(out_0->GetOpDesc(), ATTR_NAME_INNER_NET_OUTPUT, true);
  // 同一死叶子dead_2到NetOutput连两条平行控制边（模拟NetOutputPass重复连边）
  EXPECT_EQ(GraphUtils::AddEdge(dead_2->GetOutControlAnchor(), out_0->GetInControlAnchor()), GRAPH_SUCCESS);
  EXPECT_EQ(GraphUtils::AddEdge(dead_2->GetOutControlAnchor(), out_0->GetInControlAnchor()), GRAPH_SUCCESS);

  auto graph = graph_builder.GetGraph();
  PrunePass prune_pass;
  std::vector<std::pair<string, GraphPass *>> passes = {{"PrunePass", &prune_pass}};
  Status status = PassManager::Run(graph, passes);
  EXPECT_EQ(ge::SUCCESS, status);

  // 平行边全部剪除，死链dead_1/dead_2整链不可达被删
  EXPECT_EQ(graph->FindNode("dead_1"), nullptr);
  EXPECT_EQ(graph->FindNode("dead_2"), nullptr);
}

// inner NetOutput图中，原型无输入无输出的纯副作用算子（如NPUClearFloatStatusV2）没有任何
// 数据依赖可达性：不可达时须救援保活并补控制边保证执行，误删会丢失副作用
TEST_F(UtestGraphPassesPrunePass, inner_netoutput_keep_alive_no_input_output_op) {
  auto graph_builder = ut::GraphBuilder("inner_no_io");
  auto data_1 = graph_builder.AddNode("data_1", DATA, 0, 1);
  auto floor = graph_builder.AddNode("floor", FLOOR, 1, 1);
  auto clear_status = graph_builder.AddNode("clear_status", NPUCLEARFLOATSTATUSV2, 0, 0);
  auto dead_1 = graph_builder.AddNode("dead_1", REVERSE, 1, 1);
  auto out_0 = graph_builder.AddNode("out", NETOUTPUT, 1, 1);
  graph_builder.AddDataEdge(data_1, 0, floor, 0);
  graph_builder.AddDataEdge(floor, 0, out_0, 0);
  graph_builder.AddDataEdge(data_1, 0, dead_1, 0);
  AttrUtils::SetBool(out_0->GetOpDesc(), ATTR_NAME_INNER_NET_OUTPUT, true);
  EXPECT_EQ(GraphUtils::AddEdge(dead_1->GetOutControlAnchor(), out_0->GetInControlAnchor()), GRAPH_SUCCESS);

  auto graph = graph_builder.GetGraph();
  PrunePass prune_pass;
  std::vector<std::pair<string, GraphPass *>> passes = {{"PrunePass", &prune_pass}};
  Status status = PassManager::Run(graph, passes);
  EXPECT_EQ(ge::SUCCESS, status);

  // 无关死链仍被剪除
  EXPECT_EQ(graph->FindNode("dead_1"), nullptr);
  // 0输入0输出的副作用算子救援保活并补控制边
  auto clear_res = graph->FindNode("clear_status");
  ASSERT_NE(clear_res, nullptr);
  EXPECT_TRUE(clear_res->GetOutControlAnchor()->IsLinkedWith(out_0->GetInControlAnchor()));
}

// ============ 引擎感知保活判定（OptimizeStage2::PrunePass，引擎分配后第3轮剪枝）============
// 引擎已分配时：保护引擎（ge local/RTS/HostCPU/HCCL）上的节点不可剪，救援保活补控制边；
// 计算引擎上的死链照常整链剪除。引擎未分配时（第1/2轮时点）：判定退化为类型白名单，行为不变

// 引擎分配后：Stage1_3控制流降级产生的RTS骨架形态（StreamMerge/Enter，类型白名单不覆盖）落在
// RTS引擎上，不可达时须救援保活并补控制边，不能当死代码剪除；同图AI Core引擎上的纯计算死链
// 照常整链剪除——引擎判定只保护框架/调度/通信归属，不保护计算引擎
TEST_F(UtestGraphPassesPrunePass, engine_assigned_rts_skeleton_keep_alive_and_ai_core_dead_chain_pruned) {
  auto graph_builder = ut::GraphBuilder("engine_rts");
  auto data_1 = graph_builder.AddNode("data_1", DATA, 0, 1);
  auto floor = graph_builder.AddNode("floor", FLOOR, 1, 1);
  auto stream_merge = graph_builder.AddNode("stream_merge", STREAMMERGE, 2, 1);
  auto enter_1 = graph_builder.AddNode("enter_1", ENTER, 1, 1);
  auto dead_1 = graph_builder.AddNode("dead_1", REVERSE, 1, 1);
  auto dead_2 = graph_builder.AddNode("dead_2", FLOOR, 1, 1);
  auto out_0 = graph_builder.AddNode("out", NETOUTPUT, 1, 1);
  graph_builder.AddDataEdge(data_1, 0, floor, 0);
  graph_builder.AddDataEdge(floor, 0, out_0, 0);
  graph_builder.AddDataEdge(data_1, 0, stream_merge, 0);
  graph_builder.AddDataEdge(data_1, 0, stream_merge, 1);
  graph_builder.AddDataEdge(data_1, 0, enter_1, 0);
  graph_builder.AddDataEdge(data_1, 0, dead_1, 0);
  graph_builder.AddDataEdge(dead_1, 0, dead_2, 0);
  AttrUtils::SetBool(out_0->GetOpDesc(), ATTR_NAME_INNER_NET_OUTPUT, true);

  auto graph = graph_builder.GetGraph();
  // 模拟EnginePlacer分配结果：流控骨架节点归RTS引擎，死链计算节点归AI Core引擎
  stream_merge->GetOpDesc()->SetOpKernelLibName(kEngineNameRts);
  enter_1->GetOpDesc()->SetOpKernelLibName(kEngineNameRts);
  dead_1->GetOpDesc()->SetOpKernelLibName(kEngineNameAiCore);
  dead_2->GetOpDesc()->SetOpKernelLibName(kEngineNameAiCore);

  PrunePass prune_pass;
  std::vector<std::pair<string, GraphPass *>> passes = {{"PrunePass", &prune_pass}};
  Status status = PassManager::Run(graph, passes);
  EXPECT_EQ(ge::SUCCESS, status);

  // AI Core计算引擎上的死链整链剪除
  EXPECT_EQ(graph->FindNode("dead_1"), nullptr);
  EXPECT_EQ(graph->FindNode("dead_2"), nullptr);
  // RTS骨架形态救援保活并补控制边，流控结构不被破坏
  auto merge_res = graph->FindNode("stream_merge");
  ASSERT_NE(merge_res, nullptr);
  EXPECT_TRUE(merge_res->GetOutControlAnchor()->IsLinkedWith(out_0->GetInControlAnchor()));
  auto enter_res = graph->FindNode("enter_1");
  ASSERT_NE(enter_res, nullptr);
  EXPECT_TRUE(enter_res->GetOutControlAnchor()->IsLinkedWith(out_0->GetInControlAnchor()));
}

// 引擎未分配（第1/2轮剪枝时点）：GetOpKernelLibName为空，引擎判定不生效，行为与既有剪枝
// 完全一致——StreamMerge等降级后形态不在类型白名单中，不可达时照常剪除（该形态实际只在
// 第2轮之后出现；反向断言证明既有两轮行为不被引擎感知改动改变）
TEST_F(UtestGraphPassesPrunePass, engine_unassigned_behavior_same_as_legacy) {
  auto graph_builder = ut::GraphBuilder("engine_unassigned");
  auto data_1 = graph_builder.AddNode("data_1", DATA, 0, 1);
  auto floor = graph_builder.AddNode("floor", FLOOR, 1, 1);
  auto stream_merge = graph_builder.AddNode("stream_merge", STREAMMERGE, 2, 1);
  auto dead_1 = graph_builder.AddNode("dead_1", REVERSE, 1, 1);
  auto out_0 = graph_builder.AddNode("out", NETOUTPUT, 1, 1);
  graph_builder.AddDataEdge(data_1, 0, floor, 0);
  graph_builder.AddDataEdge(floor, 0, out_0, 0);
  graph_builder.AddDataEdge(data_1, 0, stream_merge, 0);
  graph_builder.AddDataEdge(data_1, 0, stream_merge, 1);
  graph_builder.AddDataEdge(data_1, 0, dead_1, 0);
  AttrUtils::SetBool(out_0->GetOpDesc(), ATTR_NAME_INNER_NET_OUTPUT, true);

  auto graph = graph_builder.GetGraph();
  PrunePass prune_pass;
  std::vector<std::pair<string, GraphPass *>> passes = {{"PrunePass", &prune_pass}};
  Status status = PassManager::Run(graph, passes);
  EXPECT_EQ(ge::SUCCESS, status);

  // 不设引擎信息：全部不可达节点按既有类型白名单语义剪除（v1行为，不变）
  EXPECT_EQ(graph->FindNode("stream_merge"), nullptr);
  EXPECT_EQ(graph->FindNode("dead_1"), nullptr);
}

// 保护引擎保守性：ge local引擎上的纯计算节点（Reshape）不可达时同样保活救援——引擎判定
// 的方向只能更保守（宁可少剪、不可错删），ge local承载图结构/输入占位语义，无法按类型
// 区分单个节点是否可剪（文档化保守取舍）
TEST_F(UtestGraphPassesPrunePass, engine_assigned_ge_local_dead_chain_kept_for_conservatism) {
  auto graph_builder = ut::GraphBuilder("engine_gelocal");
  auto data_1 = graph_builder.AddNode("data_1", DATA, 0, 1);
  auto floor = graph_builder.AddNode("floor", FLOOR, 1, 1);
  auto reshape_dead = graph_builder.AddNode("reshape_dead", RESHAPE, 1, 1);
  auto out_0 = graph_builder.AddNode("out", NETOUTPUT, 1, 1);
  graph_builder.AddDataEdge(data_1, 0, floor, 0);
  graph_builder.AddDataEdge(floor, 0, out_0, 0);
  graph_builder.AddDataEdge(data_1, 0, reshape_dead, 0);
  AttrUtils::SetBool(out_0->GetOpDesc(), ATTR_NAME_INNER_NET_OUTPUT, true);

  auto graph = graph_builder.GetGraph();
  reshape_dead->GetOpDesc()->SetOpKernelLibName(kEngineNameGeLocal);

  PrunePass prune_pass;
  std::vector<std::pair<string, GraphPass *>> passes = {{"PrunePass", &prune_pass}};
  Status status = PassManager::Run(graph, passes);
  EXPECT_EQ(ge::SUCCESS, status);

  // ge local保护引擎上的死链保守保活并补控制边（不设引擎信息时该节点本会被剪除）
  auto reshape_res = graph->FindNode("reshape_dead");
  ASSERT_NE(reshape_res, nullptr);
  EXPECT_TRUE(reshape_res->GetOutControlAnchor()->IsLinkedWith(out_0->GetInControlAnchor()));
}

// inner NetOutput图中，保活节点（Variable）输出数据边上的格式转换链（Cast等TransOp）承载
// 变量格式转换语义（后续VariableOpPass融合进变量trans road并按末端格式注册变量地址），即使
// 转换叶子空闲（控制边被拆、不可达）也须保活并补控制边；非转换类死链仍正常剪除。
// 回归背景：ST用例VariableAccSt.test_cast_fusion_with_data_copy_ok曾因Cast被剪导致变量
// 地址注册缺失（GetVarAddr失败）
TEST_F(UtestGraphPassesPrunePass, inner_netoutput_keep_alive_node_trans_closure_preserved) {
  auto graph_builder = ut::GraphBuilder("inner_trans");
  auto data_1 = graph_builder.AddNode("data_1", DATA, 0, 1);
  auto floor = graph_builder.AddNode("floor", FLOOR, 1, 1);
  auto var_1 = graph_builder.AddNode("var_1", VARIABLE, 0, 1);
  auto cast_1 = graph_builder.AddNode("cast_1", CAST, 1, 1);
  auto dead_1 = graph_builder.AddNode("dead_1", REVERSE, 1, 1);
  auto dead_2 = graph_builder.AddNode("dead_2", FLOOR, 1, 1);
  auto out_0 = graph_builder.AddNode("out", NETOUTPUT, 1, 1);
  graph_builder.AddDataEdge(data_1, 0, floor, 0);
  graph_builder.AddDataEdge(floor, 0, out_0, 0);
  graph_builder.AddDataEdge(var_1, 0, cast_1, 0);
  graph_builder.AddDataEdge(data_1, 0, dead_1, 0);
  graph_builder.AddDataEdge(dead_1, 0, dead_2, 0);
  AttrUtils::SetBool(out_0->GetOpDesc(), ATTR_NAME_INNER_NET_OUTPUT, true);
  // 模拟NetOutputPass全连叶子：转换叶子cast_1与普通死叶子dead_2均带控制边
  EXPECT_EQ(GraphUtils::AddEdge(cast_1->GetOutControlAnchor(), out_0->GetInControlAnchor()), GRAPH_SUCCESS);
  EXPECT_EQ(GraphUtils::AddEdge(dead_2->GetOutControlAnchor(), out_0->GetInControlAnchor()), GRAPH_SUCCESS);

  auto graph = graph_builder.GetGraph();
  PrunePass prune_pass;
  std::vector<std::pair<string, GraphPass *>> passes = {{"PrunePass", &prune_pass}};
  Status status = PassManager::Run(graph, passes);
  EXPECT_EQ(ge::SUCCESS, status);

  // 变量输出上的Cast承载变量格式转换语义，保活并补控制边保证执行序（融合前占位）
  auto cast_res = graph->FindNode("cast_1");
  ASSERT_NE(cast_res, nullptr);
  EXPECT_TRUE(cast_res->GetOutControlAnchor()->IsLinkedWith(out_0->GetInControlAnchor()));
  EXPECT_NE(graph->FindNode("var_1"), nullptr);
  // 非转换类死链仍整链剪除，不受转换链保活影响
  EXPECT_EQ(graph->FindNode("dead_1"), nullptr);
  EXPECT_EQ(graph->FindNode("dead_2"), nullptr);
}

// inner NetOutput图中，多输入转换算子（Reshape：idx0数据输入+idx1 shape参数输入）被保活时，
// 其参数输入生产者不可删除——否则保留的Reshape输入悬空，融合未发生场景（unknown shape/
// 多格式不一致/var-ref非法等VariableOpPass拒绝融合）下图结构不完整，算子编译失败。
// 参数输入生产链保留到keep_closure（不补控制边，执行序由到Reshape的数据依赖保证）
TEST_F(UtestGraphPassesPrunePass, inner_netoutput_multi_input_trans_op_param_input_kept) {
  auto graph_builder = ut::GraphBuilder("inner_trans_multi_input");
  auto data_1 = graph_builder.AddNode("data_1", DATA, 0, 1);
  auto floor = graph_builder.AddNode("floor", FLOOR, 1, 1);
  auto var_1 = graph_builder.AddNode("var_1", VARIABLE, 0, 1);
  auto reshape_1 = graph_builder.AddNode("reshape_1", RESHAPE, 2, 1);
  auto const_1 = graph_builder.AddNode("const_1", CONSTANT, 0, 1);
  auto dead_1 = graph_builder.AddNode("dead_1", REVERSE, 1, 1);
  auto dead_2 = graph_builder.AddNode("dead_2", FLOOR, 1, 1);
  auto out_0 = graph_builder.AddNode("out", NETOUTPUT, 1, 1);
  graph_builder.AddDataEdge(data_1, 0, floor, 0);
  graph_builder.AddDataEdge(floor, 0, out_0, 0);
  graph_builder.AddDataEdge(var_1, 0, reshape_1, 0);
  graph_builder.AddDataEdge(const_1, 0, reshape_1, 1);
  graph_builder.AddDataEdge(data_1, 0, dead_1, 0);
  graph_builder.AddDataEdge(dead_1, 0, dead_2, 0);
  AttrUtils::SetBool(out_0->GetOpDesc(), ATTR_NAME_INNER_NET_OUTPUT, true);
  // 模拟NetOutputPass全连叶子：转换叶子reshape_1与普通死叶子dead_2均带控制边
  EXPECT_EQ(GraphUtils::AddEdge(reshape_1->GetOutControlAnchor(), out_0->GetInControlAnchor()), GRAPH_SUCCESS);
  EXPECT_EQ(GraphUtils::AddEdge(dead_2->GetOutControlAnchor(), out_0->GetInControlAnchor()), GRAPH_SUCCESS);

  auto graph = graph_builder.GetGraph();
  PrunePass prune_pass;
  std::vector<std::pair<string, GraphPass *>> passes = {{"PrunePass", &prune_pass}};
  Status status = PassManager::Run(graph, passes);
  EXPECT_EQ(ge::SUCCESS, status);

  // Reshape保活并补控制边，其shape参数输入Const一并保留（数据边完整，不补控制边）
  auto reshape_res = graph->FindNode("reshape_1");
  ASSERT_NE(reshape_res, nullptr);
  EXPECT_TRUE(reshape_res->GetOutControlAnchor()->IsLinkedWith(out_0->GetInControlAnchor()));
  auto const_res = graph->FindNode("const_1");
  ASSERT_NE(const_res, nullptr);
  EXPECT_TRUE(const_res->GetOutDataAnchor(0)->IsLinkedWith(reshape_res->GetInDataAnchor(1)));
  EXPECT_FALSE(const_res->GetOutControlAnchor()->IsLinkedWith(out_0->GetInControlAnchor()));
  EXPECT_NE(graph->FindNode("var_1"), nullptr);
  // 无关死链仍整链剪除
  EXPECT_EQ(graph->FindNode("dead_1"), nullptr);
  EXPECT_EQ(graph->FindNode("dead_2"), nullptr);
}

// 多输入转换算子的参数输入为计算链（非直接Const）时，须沿入边反向完整收集生产链：
// Reshape.shape由FLOOR计算产生、FLOOR输入来自Const，两级生产节点均保留，均不补控制边
TEST_F(UtestGraphPassesPrunePass, inner_netoutput_multi_input_trans_op_param_chain_kept) {
  auto graph_builder = ut::GraphBuilder("inner_trans_param_chain");
  auto data_1 = graph_builder.AddNode("data_1", DATA, 0, 1);
  auto floor = graph_builder.AddNode("floor", FLOOR, 1, 1);
  auto var_1 = graph_builder.AddNode("var_1", VARIABLE, 0, 1);
  auto reshape_1 = graph_builder.AddNode("reshape_1", RESHAPE, 2, 1);
  auto shape_mid = graph_builder.AddNode("shape_mid", FLOOR, 1, 1);
  auto const_1 = graph_builder.AddNode("const_1", CONSTANT, 0, 1);
  auto dead_1 = graph_builder.AddNode("dead_1", REVERSE, 1, 1);
  auto dead_2 = graph_builder.AddNode("dead_2", FLOOR, 1, 1);
  auto out_0 = graph_builder.AddNode("out", NETOUTPUT, 1, 1);
  graph_builder.AddDataEdge(data_1, 0, floor, 0);
  graph_builder.AddDataEdge(floor, 0, out_0, 0);
  graph_builder.AddDataEdge(var_1, 0, reshape_1, 0);
  graph_builder.AddDataEdge(shape_mid, 0, reshape_1, 1);
  graph_builder.AddDataEdge(const_1, 0, shape_mid, 0);
  graph_builder.AddDataEdge(data_1, 0, dead_1, 0);
  graph_builder.AddDataEdge(dead_1, 0, dead_2, 0);
  AttrUtils::SetBool(out_0->GetOpDesc(), ATTR_NAME_INNER_NET_OUTPUT, true);
  EXPECT_EQ(GraphUtils::AddEdge(reshape_1->GetOutControlAnchor(), out_0->GetInControlAnchor()), GRAPH_SUCCESS);
  EXPECT_EQ(GraphUtils::AddEdge(dead_2->GetOutControlAnchor(), out_0->GetInControlAnchor()), GRAPH_SUCCESS);

  auto graph = graph_builder.GetGraph();
  PrunePass prune_pass;
  std::vector<std::pair<string, GraphPass *>> passes = {{"PrunePass", &prune_pass}};
  Status status = PassManager::Run(graph, passes);
  EXPECT_EQ(ge::SUCCESS, status);

  // Reshape保活补边；shape生产链shape_mid/const_1整链保留，数据边完整，均不补控制边
  auto reshape_res = graph->FindNode("reshape_1");
  ASSERT_NE(reshape_res, nullptr);
  EXPECT_TRUE(reshape_res->GetOutControlAnchor()->IsLinkedWith(out_0->GetInControlAnchor()));
  auto mid_res = graph->FindNode("shape_mid");
  ASSERT_NE(mid_res, nullptr);
  EXPECT_TRUE(mid_res->GetOutDataAnchor(0)->IsLinkedWith(reshape_res->GetInDataAnchor(1)));
  EXPECT_FALSE(mid_res->GetOutControlAnchor()->IsLinkedWith(out_0->GetInControlAnchor()));
  auto const_res = graph->FindNode("const_1");
  ASSERT_NE(const_res, nullptr);
  EXPECT_TRUE(const_res->GetOutDataAnchor(0)->IsLinkedWith(mid_res->GetInDataAnchor(0)));
  EXPECT_FALSE(const_res->GetOutControlAnchor()->IsLinkedWith(out_0->GetInControlAnchor()));
  EXPECT_NE(graph->FindNode("var_1"), nullptr);
  EXPECT_EQ(graph->FindNode("dead_1"), nullptr);
  EXPECT_EQ(graph->FindNode("dead_2"), nullptr);
}

// 引擎分配后轮次（Round::kAfterEngineAssigned，OptimizeStage2第3轮）对非inner NetOutput图
// （用户显式指定输出）同样生效：计算引擎（AI Core）上的不可达死链剪除；流控骨架（RTS引擎）
// 原地保留不补控制边——Stage1_3控制流降级产生的骨架从出口回溯不可达，属流水线中间产物而非
// 用户死代码，须保活。
// 回归背景：ST用例FrameworkTest.test_framework_v1_control_flow曾因第3轮无保活直接剪除降级后的
// iteration/StreamSwitch骨架导致HandleMergeInput回查next_node为空、建图失败
TEST_F(UtestGraphPassesPrunePass, engine_assigned_round_non_inner_graph_prune_only) {
  auto graph_builder = ut::GraphBuilder("engine_round_non_inner");
  auto data_1 = graph_builder.AddNode("data_1", DATA, 0, 1);
  auto floor = graph_builder.AddNode("floor", FLOOR, 1, 1);
  auto out_0 = graph_builder.AddNode("out", NETOUTPUT, 1, 1);
  auto stream_merge = graph_builder.AddNode("stream_merge", STREAMMERGE, 2, 1);
  auto dead_1 = graph_builder.AddNode("dead_1", REVERSE, 1, 1);
  graph_builder.AddDataEdge(data_1, 0, floor, 0);
  graph_builder.AddDataEdge(floor, 0, out_0, 0);
  graph_builder.AddDataEdge(data_1, 0, stream_merge, 0);
  graph_builder.AddDataEdge(data_1, 0, stream_merge, 1);
  graph_builder.AddDataEdge(data_1, 0, dead_1, 0);
  // 用户显式输出：不设ATTR_NAME_INNER_NET_OUTPUT

  auto graph = graph_builder.GetGraph();
  // 模拟EnginePlacer分配结果：流控骨架归RTS引擎，死链计算节点归AI Core引擎
  stream_merge->GetOpDesc()->SetOpKernelLibName(kEngineNameRts);
  dead_1->GetOpDesc()->SetOpKernelLibName(kEngineNameAiCore);

  // 引擎分配后轮次：非inner图AI Core死链剪除，降级骨架（RTS）原地保留且不补控制边
  PrunePass engine_round(PrunePass::Round::kAfterEngineAssigned);
  std::vector<std::pair<string, GraphPass *>> passes = {{"PrunePass", &engine_round}};
  Status status = PassManager::Run(graph, passes);
  EXPECT_EQ(ge::SUCCESS, status);
  EXPECT_EQ(graph->FindNode("dead_1"), nullptr);
  auto merge_res = graph->FindNode("stream_merge");
  ASSERT_NE(merge_res, nullptr);
  EXPECT_EQ(merge_res->GetOutControlAnchor()->GetPeerAnchors().size(), 0UL);

  // 对照：标准轮次（第1/2轮语义）同样救援保活，但补控制边保证执行序（与第3轮
  // prune-only原地保留不补边的差别）
  PrunePass standard_round;
  passes = {{"PrunePass", &standard_round}};
  status = PassManager::Run(graph, passes);
  EXPECT_EQ(ge::SUCCESS, status);
  auto merge_after = graph->FindNode("stream_merge");
  ASSERT_NE(merge_after, nullptr);
  EXPECT_TRUE(merge_after->GetOutControlAnchor()->IsLinkedWith(out_0->GetInControlAnchor()));
}

// 引擎分配后轮次（第3轮）对inner图的prune-only语义：受保护节点（流控骨架等）原地保留且
// 不补任何控制边——降级骨架依赖循环/标签语义执行，向图出口补控制边会制造虚假依赖，破坏
// hybrid执行器的数据流调度（V1 Loop死锁）；仅删除非保护引擎上的死链。
// 回归背景：ST用例DynamicGraphTest.BasicV1LoopDynamicExecSucc曾因第3轮给NextIteration/
// StreamSwitch/StreamActive补控制边到NetOutput导致执行Await超时
TEST_F(UtestGraphPassesPrunePass, engine_assigned_round_inner_graph_prune_only) {
  auto graph_builder = ut::GraphBuilder("engine_round_inner");
  auto data_1 = graph_builder.AddNode("data_1", DATA, 0, 1);
  auto floor = graph_builder.AddNode("floor", FLOOR, 1, 1);
  auto out_0 = graph_builder.AddNode("out", NETOUTPUT, 1, 1);
  auto stream_merge = graph_builder.AddNode("stream_merge", STREAMMERGE, 2, 1);
  auto dead_1 = graph_builder.AddNode("dead_1", REVERSE, 1, 1);
  auto dead_2 = graph_builder.AddNode("dead_2", FLOOR, 1, 1);
  graph_builder.AddDataEdge(data_1, 0, floor, 0);
  graph_builder.AddDataEdge(floor, 0, out_0, 0);
  graph_builder.AddDataEdge(data_1, 0, stream_merge, 0);
  graph_builder.AddDataEdge(data_1, 0, stream_merge, 1);
  graph_builder.AddDataEdge(data_1, 0, dead_1, 0);
  graph_builder.AddDataEdge(dead_1, 0, dead_2, 0);
  AttrUtils::SetBool(out_0->GetOpDesc(), ATTR_NAME_INNER_NET_OUTPUT, true);
  // 模拟NetOutputPass全连叶子：死链叶子dead_2与降级骨架叶子stream_merge均带控制边
  EXPECT_EQ(GraphUtils::AddEdge(dead_2->GetOutControlAnchor(), out_0->GetInControlAnchor()), GRAPH_SUCCESS);
  EXPECT_EQ(GraphUtils::AddEdge(stream_merge->GetOutControlAnchor(), out_0->GetInControlAnchor()), GRAPH_SUCCESS);

  auto graph = graph_builder.GetGraph();
  // 模拟EnginePlacer分配结果：流控骨架归RTS引擎，死链计算节点归AI Core引擎
  stream_merge->GetOpDesc()->SetOpKernelLibName(kEngineNameRts);
  dead_1->GetOpDesc()->SetOpKernelLibName(kEngineNameAiCore);
  dead_2->GetOpDesc()->SetOpKernelLibName(kEngineNameAiCore);

  PrunePass engine_round(PrunePass::Round::kAfterEngineAssigned);
  std::vector<std::pair<string, GraphPass *>> passes = {{"PrunePass", &engine_round}};
  Status status = PassManager::Run(graph, passes);
  EXPECT_EQ(ge::SUCCESS, status);

  // AI Core死链整链剪除（含误连控制边拆除）
  EXPECT_EQ(graph->FindNode("dead_1"), nullptr);
  EXPECT_EQ(graph->FindNode("dead_2"), nullptr);
  // RTS骨架原地保留，不新增到出口的控制边（原有控制边保留，不重复连）
  auto merge_res = graph->FindNode("stream_merge");
  ASSERT_NE(merge_res, nullptr);
  EXPECT_TRUE(merge_res->GetOutControlAnchor()->IsLinkedWith(out_0->GetInControlAnchor()));
  const auto ctrl_in_cnt = merge_res->GetOutControlAnchor()->GetPeerAnchors().size();
  EXPECT_EQ(ctrl_in_cnt, 1UL);
}
