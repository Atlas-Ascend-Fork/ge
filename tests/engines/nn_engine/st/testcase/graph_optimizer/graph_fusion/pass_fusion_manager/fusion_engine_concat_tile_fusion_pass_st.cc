/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#define protected public
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/concat_tile_fusion_pass.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "common/fe_utils.h"
#include "graph_constructor/fe_llt_utils.h"
#include "graph_constructor/fe_llt_utils.h"
#include "pass_manager.h"
#include "common/util/op_info_util.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"

#undef protected

using namespace std;
using namespace ge;
using namespace fe;

namespace fe {

class fusion_concat_tile_fusion_cast_st : public testing::Test {
 public:
  const string GRAPH_NAME = "test";
  const string CONCATV2 = "ConcatV2";
  const string CONCAT = "Concat";
  const string TILED = "TileD";
  const string TILE = "Tile";

 protected:
  void SetUp() {
    fe::InitPlatformInfo("Ascend910B2", true);
  }

  void TearDown() {}

  void InitGraph(ComputeGraphPtr &graph) {
    OpDescPtr tile1 = std::make_shared<OpDesc>("tile0", TILED);
    OpDescPtr tile2 = std::make_shared<OpDesc>("tile1", TILED);
    OpDescPtr tile3 = std::make_shared<OpDesc>("tile2", TILE);
    OpDescPtr concat = std::make_shared<OpDesc>("concat", CONCATV2);
    OpDescPtr end = std::make_shared<OpDesc>("end", "End");
    ge::AttrUtils::SetListInt(tile1, "multiples", vector<int64_t>{128, 1, 1, 1});
    ge::AttrUtils::SetListInt(tile2, "multiples", vector<int64_t>{128, 1, 1, 1});

    OpDescPtr data1 = std::make_shared<OpDesc>("data0", "Data");
    OpDescPtr data2 = std::make_shared<OpDesc>("data1", "Data");
    OpDescPtr data3 = std::make_shared<OpDesc>("data2", "Data");

    OpDescPtr const_data = std::make_shared<OpDesc>("const", "Constant");

    // add descriptor
    ge::GeShape shape0({1, 64, 7, 17777});
    GeTensorDesc input_desc0(shape0, ge::FORMAT_NCHW, ge::DT_FLOAT);
    input_desc0.SetOriginShape(shape0);
    tile1->AddInputDesc(input_desc0);
    tile2->AddInputDesc(input_desc0);
    tile3->AddInputDesc(input_desc0);

    data1->AddOutputDesc(input_desc0);
    data2->AddOutputDesc(input_desc0);
    data3->AddOutputDesc(input_desc0);

    ge::GeShape shape1({128, 64, 7, 17777});
    GeTensorDesc out_desc(shape1, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc.SetOriginShape(shape1);
    tile1->AddOutputDesc(out_desc);
    tile2->AddOutputDesc(out_desc);
    tile3->AddOutputDesc(out_desc);

    concat->AddInputDesc(out_desc);
    concat->AddInputDesc(out_desc);
    concat->AddInputDesc(out_desc);

    ge::GeShape shape2({128, 64, 21, 1777});
    GeTensorDesc out_desc2(shape2, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc2.SetOriginShape(shape2);
    concat->AddOutputDesc(out_desc2);
    end->AddInputDesc(out_desc2);

    ge::GeShape shape3({1});
    GeTensorDesc out_desc3(shape2, ge::FORMAT_ND, ge::DT_INT32);
    concat->AddInputDesc(out_desc3);
    const_data->AddOutputDesc(out_desc3);

    NodePtr tile1_node = graph->AddNode(tile1);
    NodePtr tile2_node = graph->AddNode(tile2);
    NodePtr tile3_node = graph->AddNode(tile3);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr data3_node = graph->AddNode(data3);
    NodePtr concat_node = graph->AddNode(concat);
    NodePtr const_op = graph->AddNode(const_data);

    NodePtr end_node = graph->AddNode(end);

    ge::GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0), tile1_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(data2_node->GetOutDataAnchor(0), tile2_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(data3_node->GetOutDataAnchor(0), tile3_node->GetInDataAnchor(0));

    ge::GraphUtils::AddEdge(tile1_node->GetOutDataAnchor(0), concat_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(tile2_node->GetOutDataAnchor(0), concat_node->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(tile3_node->GetOutDataAnchor(0), concat_node->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(const_op->GetOutDataAnchor(0), concat_node->GetInDataAnchor(3));
    ge::GraphUtils::AddEdge(concat_node->GetOutDataAnchor(0), end_node->GetInDataAnchor(0));
  }

  void InitGraph1(ComputeGraphPtr &graph) {
    OpDescPtr tile1 = std::make_shared<OpDesc>("tile0", TILED);
    OpDescPtr tile2 = std::make_shared<OpDesc>("tile1", TILED);
    OpDescPtr tile3 = std::make_shared<OpDesc>("tile2", TILE);
    OpDescPtr concat = std::make_shared<OpDesc>("concat", CONCATV2);
    OpDescPtr end = std::make_shared<OpDesc>("end", "End");
    ge::AttrUtils::SetListInt(tile1, "multiples", vector<int64_t>{128, 1, 1, 1});
    ge::AttrUtils::SetListInt(tile2, "multiples", vector<int64_t>{128, 1, 1, 1});

    OpDescPtr data1 = std::make_shared<OpDesc>("data0", "Data");
    OpDescPtr data2 = std::make_shared<OpDesc>("data1", "Data");
    OpDescPtr data3 = std::make_shared<OpDesc>("data2", "Data");

    OpDescPtr const_data = std::make_shared<OpDesc>("const", "Constant");

    // add descriptor
    ge::GeShape shape0({1, 6, 1, 1});
    GeTensorDesc input_desc0(shape0, ge::FORMAT_NCHW, ge::DT_FLOAT);
    input_desc0.SetOriginShape(shape0);
    tile1->AddInputDesc(input_desc0);
    tile2->AddInputDesc(input_desc0);
    tile3->AddInputDesc(input_desc0);
    data1->AddOutputDesc(input_desc0);
    data2->AddOutputDesc(input_desc0);
    data3->AddOutputDesc(input_desc0);

    ge::GeShape shape1({128, 6, 1, 1});
    GeTensorDesc out_desc(shape1, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc.SetOriginShape(shape1);
    tile1->AddOutputDesc(out_desc);
    tile2->AddOutputDesc(out_desc);
    tile3->AddOutputDesc(out_desc);

    concat->AddInputDesc(out_desc);
    concat->AddInputDesc(out_desc);
    concat->AddInputDesc(out_desc);

    ge::GeShape shape2({128, 6, 3, 1});
    GeTensorDesc out_desc2(shape2, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc2.SetOriginShape(shape2);
    concat->AddOutputDesc(out_desc2);
    end->AddInputDesc(out_desc2);

    ge::GeShape shape3({1});
    GeTensorDesc out_desc3(shape2, ge::FORMAT_ND, ge::DT_INT32);
    concat->AddInputDesc(out_desc3);
    const_data->AddOutputDesc(out_desc3);

    NodePtr tile1_node = graph->AddNode(tile1);
    NodePtr tile2_node = graph->AddNode(tile2);
    NodePtr tile3_node = graph->AddNode(tile3);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr data3_node = graph->AddNode(data3);
    NodePtr concat_node = graph->AddNode(concat);
    NodePtr const_op = graph->AddNode(const_data);

    NodePtr end_node = graph->AddNode(end);

    ge::GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0), tile1_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(data2_node->GetOutDataAnchor(0), tile2_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(data3_node->GetOutDataAnchor(0), tile3_node->GetInDataAnchor(0));

    ge::GraphUtils::AddEdge(tile1_node->GetOutDataAnchor(0), concat_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(tile2_node->GetOutDataAnchor(0), concat_node->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(tile3_node->GetOutDataAnchor(0), concat_node->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(const_op->GetOutDataAnchor(0), concat_node->GetInDataAnchor(3));
    ge::GraphUtils::AddEdge(concat_node->GetOutDataAnchor(0), end_node->GetInDataAnchor(0));
  }

  void InitGraph2(ComputeGraphPtr &graph) {
    OpDescPtr tile1 = std::make_shared<OpDesc>("tile0", TILED);
    OpDescPtr tile2 = std::make_shared<OpDesc>("tile1", TILED);
    OpDescPtr concat = std::make_shared<OpDesc>("concat", CONCATV2);
    OpDescPtr end = std::make_shared<OpDesc>("end", "End");
    ge::AttrUtils::SetListInt(tile1, "multiples", vector<int64_t>{128, 1, 1, 1});
    ge::AttrUtils::SetListInt(tile2, "multiples", vector<int64_t>{128, 1, 1, 1});

    OpDescPtr data1 = std::make_shared<OpDesc>("data0", "Data");
    OpDescPtr data2 = std::make_shared<OpDesc>("data1", "Data");

    OpDescPtr const_data = std::make_shared<OpDesc>("const", "Constant");

    // add descriptor
    ge::GeShape shape0({1, 6, 1, 1});
    GeTensorDesc input_desc0(shape0, ge::FORMAT_NCHW, ge::DT_FLOAT);
    input_desc0.SetOriginShape(shape0);
    tile1->AddInputDesc(input_desc0);
    tile2->AddInputDesc(input_desc0);
    data1->AddOutputDesc(input_desc0);
    data2->AddOutputDesc(input_desc0);

    ge::GeShape shape1({128, 6, 1, 1});
    GeTensorDesc out_desc(shape1, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc.SetOriginShape(shape1);
    tile1->AddOutputDesc(out_desc);
    tile2->AddOutputDesc(out_desc);

    concat->AddInputDesc(out_desc);
    concat->AddInputDesc(out_desc);

    ge::GeShape shape2({128, 6, 2, 1});
    GeTensorDesc out_desc2(shape2, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc2.SetOriginShape(shape2);
    concat->AddOutputDesc(out_desc2);
    end->AddInputDesc(out_desc2);

    ge::GeShape shape3({1});
    GeTensorDesc out_desc3(shape2, ge::FORMAT_ND, ge::DT_INT32);
    concat->AddInputDesc(out_desc3);
    const_data->AddOutputDesc(out_desc3);

    NodePtr tile1_node = graph->AddNode(tile1);
    NodePtr tile2_node = graph->AddNode(tile2);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr concat_node = graph->AddNode(concat);
    NodePtr const_op = graph->AddNode(const_data);

    NodePtr end_node = graph->AddNode(end);

    ge::GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0), tile1_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(data2_node->GetOutDataAnchor(0), tile2_node->GetInDataAnchor(0));

    ge::GraphUtils::AddEdge(tile1_node->GetOutDataAnchor(0), concat_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(tile2_node->GetOutDataAnchor(0), concat_node->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(const_op->GetOutDataAnchor(0), concat_node->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(concat_node->GetOutDataAnchor(0), end_node->GetInDataAnchor(0));
  }

  void InitGraph3(ComputeGraphPtr &graph) {
    OpDescPtr tile1 = std::make_shared<OpDesc>("tile0", TILED);
    OpDescPtr tile2 = std::make_shared<OpDesc>("tile1", TILED);
    OpDescPtr tile3 = std::make_shared<OpDesc>("tile2", TILED);

    OpDescPtr tile4 = std::make_shared<OpDesc>("tile3", TILED);
    OpDescPtr tile5 = std::make_shared<OpDesc>("tile4", TILED);
    OpDescPtr tile6 = std::make_shared<OpDesc>("tile5", TILED);
    ge::AttrUtils::SetListInt(tile1, "multiples", vector<int64_t>{128, 1, 1, 1});
    ge::AttrUtils::SetListInt(tile2, "multiples", vector<int64_t>{128, 1, 1, 1});
    ge::AttrUtils::SetListInt(tile3, "multiples", vector<int64_t>{128, 1, 1, 1});

    ge::AttrUtils::SetListInt(tile4, "multiples", vector<int64_t>{64, 1, 1, 1});
    ge::AttrUtils::SetListInt(tile5, "multiples", vector<int64_t>{64, 1, 1, 1});
    ge::AttrUtils::SetListInt(tile6, "multiples", vector<int64_t>{64, 1, 1, 1});

    OpDescPtr concat = std::make_shared<OpDesc>("concat", CONCATV2);
    OpDescPtr end = std::make_shared<OpDesc>("end", "End");

    OpDescPtr data1 = std::make_shared<OpDesc>("data0", "Data");
    OpDescPtr data2 = std::make_shared<OpDesc>("data1", "Data");
    OpDescPtr data3 = std::make_shared<OpDesc>("data2", "Data");

    OpDescPtr data4 = std::make_shared<OpDesc>("data3", "Data");
    OpDescPtr data5 = std::make_shared<OpDesc>("data4", "Data");
    OpDescPtr data6 = std::make_shared<OpDesc>("data5", "Data");

    OpDescPtr const_data = std::make_shared<OpDesc>("const", "Constant");

    // add descriptor
    ge::GeShape shape0({1, 6, 1, 1});
    GeTensorDesc input_desc0(shape0, ge::FORMAT_NCHW, ge::DT_FLOAT);
    input_desc0.SetOriginShape(shape0);
    tile1->AddInputDesc(input_desc0);
    tile2->AddInputDesc(input_desc0);
    tile3->AddInputDesc(input_desc0);

    data1->AddOutputDesc(input_desc0);
    data2->AddOutputDesc(input_desc0);
    data3->AddOutputDesc(input_desc0);

    ge::GeShape shape_0({2, 6, 1, 1});
    GeTensorDesc input_desc_0(shape_0, ge::FORMAT_NCHW, ge::DT_FLOAT);
    input_desc_0.SetOriginShape(shape_0);
    tile4->AddInputDesc(input_desc_0);
    tile5->AddInputDesc(input_desc_0);
    tile6->AddInputDesc(input_desc_0);

    data4->AddOutputDesc(input_desc_0);
    data5->AddOutputDesc(input_desc_0);
    data6->AddOutputDesc(input_desc_0);

    ge::GeShape shape1({128, 6, 6, 1});
    GeTensorDesc out_desc(shape1, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc.SetOriginShape(shape1);
    tile1->AddOutputDesc(out_desc);
    tile2->AddOutputDesc(out_desc);
    tile3->AddOutputDesc(out_desc);

    tile4->AddOutputDesc(out_desc);
    tile5->AddOutputDesc(out_desc);
    tile6->AddOutputDesc(out_desc);

    concat->AddInputDesc(out_desc);
    concat->AddInputDesc(out_desc);
    concat->AddInputDesc(out_desc);

    concat->AddInputDesc(out_desc);
    concat->AddInputDesc(out_desc);
    concat->AddInputDesc(out_desc);

    ge::GeShape shape2({128, 6, 42, 7});
    GeTensorDesc out_desc2(shape2, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc2.SetOriginShape(shape2);
    concat->AddOutputDesc(out_desc2);
    end->AddInputDesc(out_desc2);

    ge::GeShape shape3({1});
    GeTensorDesc out_desc3(shape2, ge::FORMAT_ND, ge::DT_INT32);
    concat->AddInputDesc(out_desc3);
    const_data->AddOutputDesc(out_desc3);

    NodePtr tile1_node = graph->AddNode(tile1);
    NodePtr tile2_node = graph->AddNode(tile2);
    NodePtr tile3_node = graph->AddNode(tile3);
    NodePtr tile4_node = graph->AddNode(tile4);
    NodePtr tile5_node = graph->AddNode(tile5);
    NodePtr tile6_node = graph->AddNode(tile6);

    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr data3_node = graph->AddNode(data3);
    NodePtr data4_node = graph->AddNode(data4);
    NodePtr data5_node = graph->AddNode(data5);
    NodePtr data6_node = graph->AddNode(data6);

    NodePtr concat_node = graph->AddNode(concat);
    NodePtr const_op = graph->AddNode(const_data);

    NodePtr end_node = graph->AddNode(end);

    ge::GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0), tile1_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(data2_node->GetOutDataAnchor(0), tile2_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(data3_node->GetOutDataAnchor(0), tile3_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(data4_node->GetOutDataAnchor(0), tile4_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(data5_node->GetOutDataAnchor(0), tile5_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(data6_node->GetOutDataAnchor(0), tile6_node->GetInDataAnchor(0));

    ge::GraphUtils::AddEdge(tile1_node->GetOutDataAnchor(0), concat_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(tile2_node->GetOutDataAnchor(0), concat_node->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(tile3_node->GetOutDataAnchor(0), concat_node->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(tile4_node->GetOutDataAnchor(0), concat_node->GetInDataAnchor(3));
    ge::GraphUtils::AddEdge(tile5_node->GetOutDataAnchor(0), concat_node->GetInDataAnchor(4));
    ge::GraphUtils::AddEdge(tile6_node->GetOutDataAnchor(0), concat_node->GetInDataAnchor(5));

    ge::GraphUtils::AddEdge(const_op->GetOutDataAnchor(0), concat_node->GetInDataAnchor(6));
    ge::GraphUtils::AddEdge(concat_node->GetOutDataAnchor(0), end_node->GetInDataAnchor(0));
  }

  void InitGraph4(ComputeGraphPtr &graph) {
    OpDescPtr tile1 = std::make_shared<OpDesc>("tile0", TILED);
    OpDescPtr tile2 = std::make_shared<OpDesc>("tile1", TILED);
    OpDescPtr tile3 = std::make_shared<OpDesc>("tile2", TILED);

    OpDescPtr tile4 = std::make_shared<OpDesc>("tile3", TILED);
    OpDescPtr tile5 = std::make_shared<OpDesc>("tile4", TILED);
    OpDescPtr tile6 = std::make_shared<OpDesc>("tile5", TILED);
    ge::AttrUtils::SetListInt(tile1, "multiples", vector<int64_t>{128, 1, 1, 1});
    ge::AttrUtils::SetListInt(tile2, "multiples", vector<int64_t>{128, 1, 1, 1});
    ge::AttrUtils::SetListInt(tile3, "multiples", vector<int64_t>{128, 1, 1, 1});

    ge::AttrUtils::SetListInt(tile4, "multiples", vector<int64_t>{128, 1, 1, 1});
    ge::AttrUtils::SetListInt(tile5, "multiples", vector<int64_t>{128, 1, 1, 1});
    ge::AttrUtils::SetListInt(tile6, "multiples", vector<int64_t>{128, 1, 1, 1});

    OpDescPtr concat = std::make_shared<OpDesc>("concat", CONCATV2);
    OpDescPtr end = std::make_shared<OpDesc>("end", "End");

    OpDescPtr data1 = std::make_shared<OpDesc>("data0", "Data");
    OpDescPtr data2 = std::make_shared<OpDesc>("data1", "Data");
    OpDescPtr data3 = std::make_shared<OpDesc>("data2", "Data");

    OpDescPtr data4 = std::make_shared<OpDesc>("data3", "Data");
    OpDescPtr data5 = std::make_shared<OpDesc>("data4", "Data");
    OpDescPtr data6 = std::make_shared<OpDesc>("data5", "Data");
    OpDescPtr data7 = std::make_shared<OpDesc>("data6", "Data");

    OpDescPtr const_data = std::make_shared<OpDesc>("const", "Constant");
    OpDescPtr contrl_data = std::make_shared<OpDesc>("contrlconst", "Constant");

    // add descriptor
    ge::GeShape shape0({1, 6, 1, 1});
    GeTensorDesc input_desc0(shape0, ge::FORMAT_NCHW, ge::DT_FLOAT);
    input_desc0.SetOriginShape(shape0);
    tile1->AddInputDesc(input_desc0);
    tile2->AddInputDesc(input_desc0);
    tile3->AddInputDesc(input_desc0);

    data1->AddOutputDesc(input_desc0);
    data2->AddOutputDesc(input_desc0);
    data3->AddOutputDesc(input_desc0);

    ge::GeShape shape_0({1, 6, 1, 1});
    GeTensorDesc input_desc_0(shape_0, ge::FORMAT_NCHW, ge::DT_FLOAT);
    input_desc_0.SetOriginShape(shape_0);
    tile4->AddInputDesc(input_desc_0);
    tile5->AddInputDesc(input_desc_0);
    tile6->AddInputDesc(input_desc_0);

    data4->AddOutputDesc(input_desc_0);
    data5->AddOutputDesc(input_desc_0);
    data6->AddOutputDesc(input_desc_0);

    ge::GeShape shape1({128, 6, 6, 1});
    GeTensorDesc out_desc(shape1, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc.SetOriginShape(shape1);
    tile1->AddOutputDesc(out_desc);
    tile2->AddOutputDesc(out_desc);
    tile3->AddOutputDesc(out_desc);

    tile4->AddOutputDesc(out_desc);
    tile5->AddOutputDesc(out_desc);
    tile6->AddOutputDesc(out_desc);

    concat->AddInputDesc(out_desc);
    concat->AddInputDesc(out_desc);
    concat->AddInputDesc(out_desc);

    concat->AddInputDesc(out_desc);
    concat->AddInputDesc(out_desc);
    concat->AddInputDesc(out_desc);
    concat->AddInputDesc(out_desc);
    data7->AddOutputDesc(out_desc);

    ge::GeShape shape2({128, 6, 42, 7});
    GeTensorDesc out_desc2(shape2, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc2.SetOriginShape(shape2);
    concat->AddOutputDesc(out_desc2);
    end->AddInputDesc(out_desc2);

    ge::GeShape shape3({1});
    GeTensorDesc out_desc3(shape2, ge::FORMAT_ND, ge::DT_INT32);
    concat->AddInputDesc(out_desc3);
    const_data->AddOutputDesc(out_desc3);

    NodePtr tile1_node = graph->AddNode(tile1);
    NodePtr tile2_node = graph->AddNode(tile2);
    NodePtr tile3_node = graph->AddNode(tile3);
    NodePtr tile4_node = graph->AddNode(tile4);
    NodePtr tile5_node = graph->AddNode(tile5);
    NodePtr tile6_node = graph->AddNode(tile6);

    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr data3_node = graph->AddNode(data3);
    NodePtr data4_node = graph->AddNode(data4);
    NodePtr data5_node = graph->AddNode(data5);
    NodePtr data6_node = graph->AddNode(data6);
    NodePtr data7_node = graph->AddNode(data7);

    NodePtr concat_node = graph->AddNode(concat);
    NodePtr const_op = graph->AddNode(const_data);
    NodePtr contrl_op = graph->AddNode(contrl_data);

    NodePtr end_node = graph->AddNode(end);

    ge::GraphUtils::AddEdge(contrl_op->GetOutControlAnchor(), tile1_node->GetInControlAnchor());
    ge::GraphUtils::AddEdge(contrl_op->GetOutControlAnchor(), tile2_node->GetInControlAnchor());
    ge::GraphUtils::AddEdge(contrl_op->GetOutControlAnchor(), tile3_node->GetInControlAnchor());

    ge::GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0), tile1_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(data2_node->GetOutDataAnchor(0), tile2_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(data3_node->GetOutDataAnchor(0), tile3_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(data4_node->GetOutDataAnchor(0), tile4_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(data5_node->GetOutDataAnchor(0), tile5_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(data6_node->GetOutDataAnchor(0), tile6_node->GetInDataAnchor(0));

    ge::GraphUtils::AddEdge(tile1_node->GetOutDataAnchor(0), concat_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(tile2_node->GetOutDataAnchor(0), concat_node->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(tile3_node->GetOutDataAnchor(0), concat_node->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(data7_node->GetOutDataAnchor(0), concat_node->GetInDataAnchor(3));
    ge::GraphUtils::AddEdge(tile4_node->GetOutDataAnchor(0), concat_node->GetInDataAnchor(4));
    ge::GraphUtils::AddEdge(tile5_node->GetOutDataAnchor(0), concat_node->GetInDataAnchor(5));
    ge::GraphUtils::AddEdge(tile6_node->GetOutDataAnchor(0), concat_node->GetInDataAnchor(6));

    ge::GraphUtils::AddEdge(const_op->GetOutDataAnchor(0), concat_node->GetInDataAnchor(7));
    ge::GraphUtils::AddEdge(concat_node->GetOutDataAnchor(0), end_node->GetInDataAnchor(0));
  }

  void InitGraph6(ComputeGraphPtr &graph, bool flag = false) {
    OpDescPtr tile1 = std::make_shared<OpDesc>("tile0", TILED);
    OpDescPtr tile2 = std::make_shared<OpDesc>("tile1", TILED);
    OpDescPtr tile3 = std::make_shared<OpDesc>("tile2", TILE);
    OpDescPtr concat = std::make_shared<OpDesc>("concat", CONCATV2);
    OpDescPtr end = std::make_shared<OpDesc>("end", "End");
    OpDescPtr end1 = std::make_shared<OpDesc>("end1", "End");
    ge::AttrUtils::SetListInt(tile1, "multiples", vector<int64_t>{128, 1, 1, 4});
    ge::AttrUtils::SetListInt(tile2, "multiples", vector<int64_t>{128, 1, 1, 4});
    ge::AttrUtils::SetListInt(tile3, "multiples", vector<int64_t>{128, 1, 1, 4});

    OpDescPtr data1 = std::make_shared<OpDesc>("data0", "Data");
    OpDescPtr data2 = std::make_shared<OpDesc>("data1", "Data");
    OpDescPtr data3 = std::make_shared<OpDesc>("data2", "Data");

    OpDescPtr const_data = std::make_shared<OpDesc>("const", "Constant");

    // add descriptor
    ge::GeShape shape0({1, 6, 1, 1});
    GeTensorDesc input_desc0(shape0, ge::FORMAT_NCHW, ge::DT_FLOAT);
    input_desc0.SetOriginShape(shape0);
    tile1->AddInputDesc(input_desc0);
    tile2->AddInputDesc(input_desc0);
    tile3->AddInputDesc(input_desc0);
    data1->AddOutputDesc(input_desc0);
    data2->AddOutputDesc(input_desc0);
    data3->AddOutputDesc(input_desc0);

    ge::GeShape shape1({4, 6, 4, 4});
    GeTensorDesc out_desc(shape1, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc.SetOriginShape(shape1);
    tile1->AddOutputDesc(out_desc);
    tile2->AddOutputDesc(out_desc);
    tile3->AddOutputDesc(out_desc);

    concat->AddInputDesc(out_desc);
    concat->AddInputDesc(out_desc);
    concat->AddInputDesc(out_desc);

    ge::GeShape shape2({128, 6, 3, 8});
    GeTensorDesc out_desc2(shape2, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc2.SetOriginShape(shape2);
    concat->AddOutputDesc(out_desc2);
    end->AddInputDesc(out_desc2);
    end1->AddInputDesc(out_desc2);

    ge::GeShape shape3({1});
    GeTensorDesc out_desc3(shape2, ge::FORMAT_ND, ge::DT_INT32);
    concat->AddInputDesc(out_desc3);
    const_data->AddOutputDesc(out_desc3);

    NodePtr tile1_node = graph->AddNode(tile1);
    NodePtr tile2_node = graph->AddNode(tile2);
    NodePtr tile3_node = graph->AddNode(tile3);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr data3_node = graph->AddNode(data3);
    NodePtr concat_node = graph->AddNode(concat);
    NodePtr const_op = graph->AddNode(const_data);

    NodePtr end_node = graph->AddNode(end);
    NodePtr end_node1 = graph->AddNode(end1);

    ge::GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0), tile1_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(data2_node->GetOutDataAnchor(0), tile2_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(data3_node->GetOutDataAnchor(0), tile3_node->GetInDataAnchor(0));

    ge::GraphUtils::AddEdge(tile1_node->GetOutDataAnchor(0), concat_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(tile2_node->GetOutDataAnchor(0), concat_node->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(tile3_node->GetOutDataAnchor(0), concat_node->GetInDataAnchor(2));
    if (flag) {
      ge::GraphUtils::AddEdge(tile3_node->GetOutDataAnchor(0), end_node1->GetInDataAnchor(0));
    }
    ge::GraphUtils::AddEdge(const_op->GetOutDataAnchor(0), concat_node->GetInDataAnchor(3));
    ge::GraphUtils::AddEdge(concat_node->GetOutDataAnchor(0), end_node->GetInDataAnchor(0));
  }

  static void DumpGraph(const ge::ComputeGraphPtr graph, string graph_name) {
    printf("start to dump graph %s...\n", graph_name.c_str());
    for (ge::NodePtr node : graph->GetAllNodes()) {
      printf("node name = %s.\n", node->GetName().c_str());
      for (ge::OutDataAnchorPtr anchor : node->GetAllOutDataAnchors()) {
        for (ge::InDataAnchorPtr peer_in_anchor : anchor->GetPeerInDataAnchors()) {
          printf("    node name = %s[%d], out data node name = %s[%d].\n", node->GetName().c_str(), anchor->GetIdx(),
                 peer_in_anchor->GetOwnerNode()->GetName().c_str(), peer_in_anchor->GetIdx());
        }
      }
      if (node->GetOutControlAnchor() != nullptr) {
        for (ge::InControlAnchorPtr peer_in_anchor : node->GetOutControlAnchor()->GetPeerInControlAnchors()) {
          printf("    node name = %s, out control node name = %s.\n", node->GetName().c_str(),
                 peer_in_anchor->GetOwnerNode()->GetName().c_str());
        }
      }
    }

    return;
  }
};

TEST_F(fusion_concat_tile_fusion_cast_st, concat_tile_not_changed0) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  InitGraph(graph);
  ConcatTileFusionPass pass;
  vector<ge::NodePtr> fusion_nodes = {};
  PatternFusionBasePass::Mapping mapping;
  std::shared_ptr<FusionPattern::OpDesc> op_desc1 = std::make_shared<FusionPattern::OpDesc>();
  op_desc1->id = "concat";
  NodePtr node1 = graph->FindNode("concat");
  mapping[op_desc1] = std::vector<ge::NodePtr>{node1};
  Status status = pass.Fusion(*graph, mapping, fusion_nodes);
  DumpGraph(graph, "test");
  EXPECT_EQ(fe::NOT_CHANGED, status);
}

TEST_F(fusion_concat_tile_fusion_cast_st, concat_tile_Success0) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  InitGraph1(graph);
  ConcatTileFusionPass pass;
  vector<ge::NodePtr> fusion_nodes = {};
  PatternFusionBasePass::Mapping mapping;
  std::shared_ptr<FusionPattern::OpDesc> op_desc1 = std::make_shared<FusionPattern::OpDesc>();
  op_desc1->id = "concat";
  NodePtr node1 = graph->FindNode("concat");
  mapping[op_desc1] = std::vector<ge::NodePtr>{node1};
  Status status = pass.Fusion(*graph, mapping, fusion_nodes);
  DumpGraph(graph, "test");
  EXPECT_EQ(fe::SUCCESS, status);
}

TEST_F(fusion_concat_tile_fusion_cast_st, concat_tile_not_changed1) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  InitGraph2(graph);
  ConcatTileFusionPass pass;
  vector<ge::NodePtr> fusion_nodes = {};
  PatternFusionBasePass::Mapping mapping;
  std::shared_ptr<FusionPattern::OpDesc> op_desc1 = std::make_shared<FusionPattern::OpDesc>();
  op_desc1->id = "concat";
  NodePtr node1 = graph->FindNode("concat");
  mapping[op_desc1] = std::vector<ge::NodePtr>{node1};
  Status status = pass.Fusion(*graph, mapping, fusion_nodes);
  DumpGraph(graph, "test");
  EXPECT_EQ(fe::NOT_CHANGED, status);
}

TEST_F(fusion_concat_tile_fusion_cast_st, concat_tile_Success1) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  InitGraph3(graph);
  ConcatTileFusionPass pass;
  vector<ge::NodePtr> fusion_nodes = {};
  PatternFusionBasePass::Mapping mapping;
  std::shared_ptr<FusionPattern::OpDesc> op_desc1 = std::make_shared<FusionPattern::OpDesc>();
  op_desc1->id = "concat";
  NodePtr node1 = graph->FindNode("concat");
  mapping[op_desc1] = std::vector<ge::NodePtr>{node1};
  Status status = pass.Fusion(*graph, mapping, fusion_nodes);
  DumpGraph(graph, "test");
  EXPECT_EQ(fe::SUCCESS, status);
}

TEST_F(fusion_concat_tile_fusion_cast_st, concat_tile_Success2) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  InitGraph4(graph);
  ConcatTileFusionPass pass;
  vector<ge::NodePtr> fusion_nodes = {};
  PatternFusionBasePass::Mapping mapping;
  std::shared_ptr<FusionPattern::OpDesc> op_desc1 = std::make_shared<FusionPattern::OpDesc>();
  op_desc1->id = "concat";
  NodePtr node1 = graph->FindNode("concat");
  mapping[op_desc1] = std::vector<ge::NodePtr>{node1};
  Status status = pass.Fusion(*graph, mapping, fusion_nodes);
  DumpGraph(graph, "test");
  EXPECT_EQ(fe::SUCCESS, status);
}

TEST_F(fusion_concat_tile_fusion_cast_st, concat_tile_Failed01) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  InitGraph6(graph);
  ConcatTileFusionPass pass;
  vector<ge::NodePtr> fusion_nodes = {};
  PatternFusionBasePass::Mapping mapping;
  std::shared_ptr<FusionPattern::OpDesc> op_desc1 = std::make_shared<FusionPattern::OpDesc>();
  op_desc1->id = "concat";
  NodePtr node1 = graph->FindNode("concat");
  mapping[op_desc1] = std::vector<ge::NodePtr>{node1};
  Status status = pass.Fusion(*graph, mapping, fusion_nodes);
  DumpGraph(graph, "test");
}

TEST_F(fusion_concat_tile_fusion_cast_st, parse_concat_node_without_concat_dim_attr) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  OpDescPtr concat = std::make_shared<OpDesc>("concat", "ConcatD");
  GeShape shape({2});
  GeTensorDesc tensor_desc(shape, ge::FORMAT_NCHW, ge::DT_FLOAT);
  tensor_desc.SetOriginShape(shape);
  concat->AddInputDesc(tensor_desc);
  concat->AddOutputDesc(tensor_desc);
  NodePtr concat_node = graph->AddNode(concat);
  ConcatTileFusionPass pass;
  EXPECT_EQ(fe::NOT_CHANGED, pass.ParseConcatNode(concat_node));
}

TEST_F(fusion_concat_tile_fusion_cast_st, update_concat_tile_tensor_invalid_input_shape) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  OpDescPtr tile = std::make_shared<OpDesc>("tile", TILED);
  GeShape shape({1, 2, 3, 4});
  GeTensorDesc tensor_desc(shape, ge::FORMAT_NCHW, ge::DT_FLOAT);
  tensor_desc.SetOriginShape(shape);
  tile->AddOutputDesc(tensor_desc);
  NodePtr tile_node = graph->AddNode(tile);
  OpDescPtr new_concat = std::make_shared<OpDesc>("new_concat", CONCATV2);
  ConcatTileFusionPass pass;
  pass.concat_dim_ = 4;
  GeTensorDesc input_output_tensor = tile_node->GetOpDesc()->GetOutputDesc(0);
  EXPECT_EQ(fe::FAILED, pass.UpdateConcatTileTensor(new_concat, tile_node, 4, 4, input_output_tensor));
}

TEST_F(fusion_concat_tile_fusion_cast_st, update_concat_tile_tensor_invalid_tile_shape) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  OpDescPtr tile = std::make_shared<OpDesc>("tile", TILED);
  GeShape data_shape({1, 2, 3, 4});
  GeTensorDesc input_output_tensor(data_shape, ge::FORMAT_NCHW, ge::DT_FLOAT);
  input_output_tensor.SetOriginShape(data_shape);
  GeShape tile_shape({1, 2});
  GeTensorDesc tile_tensor_desc(tile_shape, ge::FORMAT_NCHW, ge::DT_FLOAT);
  tile->AddOutputDesc(tile_tensor_desc);
  NodePtr tile_node = graph->AddNode(tile);
  OpDescPtr new_concat = std::make_shared<OpDesc>("new_concat", CONCATV2);
  ConcatTileFusionPass pass;
  pass.concat_dim_ = 2;
  EXPECT_EQ(fe::FAILED, pass.UpdateConcatTileTensor(new_concat, tile_node, 4, 4, input_output_tensor));
}

TEST_F(fusion_concat_tile_fusion_cast_st, get_concat_value_by_concatdim_shape_out_of_range) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  OpDescPtr tile = std::make_shared<OpDesc>("tile", TILED);
  GeShape data_shape({1, 2, 3, 4});
  GeShape ori_shape({1, 2});
  GeTensorDesc tensor_desc(data_shape, ge::FORMAT_NCHW, ge::DT_FLOAT);
  tensor_desc.SetOriginShape(ori_shape);
  tile->AddOutputDesc(tensor_desc);
  NodePtr tile_node = graph->AddNode(tile);
  OpDescPtr new_concat = std::make_shared<OpDesc>("new_concat", CONCATV2);
  ConcatTileFusionPass pass;
  vector<ge::NodePtr> inputs = {tile_node};
  int64_t concat_shape_dim = 0;
  int64_t concat_ori_shape_dim = 0;
  pass.concat_dim_ = 3;
  EXPECT_EQ(fe::FAILED, pass.GetConcatValueByConcatdim(inputs, concat_shape_dim, concat_ori_shape_dim, new_concat));
  pass.concat_dim_ = -1;
  EXPECT_EQ(fe::FAILED, pass.GetConcatValueByConcatdim(inputs, concat_shape_dim, concat_ori_shape_dim, new_concat));
}

TEST_F(fusion_concat_tile_fusion_cast_st, insert_concat_op_with_empty_inputs) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  OpDescPtr tile = std::make_shared<OpDesc>("tile", TILED);
  GeShape shape({1, 2});
  GeTensorDesc tensor_desc(shape, ge::FORMAT_NCHW, ge::DT_FLOAT);
  tile->AddInputDesc(tensor_desc);
  tile->AddOutputDesc(tensor_desc);
  NodePtr tile_node = graph->AddNode(tile);
  OpDescPtr new_concat = std::make_shared<OpDesc>("new_concat", CONCATV2);
  ConcatTileFusionPass pass;
  vector<ge::NodePtr> inputs;
  EXPECT_EQ(fe::FAILED, pass.InsertConcatOp(new_concat, inputs, tile_node));
}

TEST_F(fusion_concat_tile_fusion_cast_st, do_tile_fusion_tile_without_peer) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  OpDescPtr tile = std::make_shared<OpDesc>("tile", TILED);
  GeShape shape({1, 2});
  GeTensorDesc tensor_desc(shape, ge::FORMAT_NCHW, ge::DT_FLOAT);
  tile->AddInputDesc(tensor_desc);
  tile->AddOutputDesc(tensor_desc);
  NodePtr tile_node = graph->AddNode(tile);
  OpDescPtr concat = std::make_shared<OpDesc>("concat", CONCATV2);
  concat->AddInputDesc(tensor_desc);
  concat->AddOutputDesc(tensor_desc);
  NodePtr concat_node = graph->AddNode(concat);
  OpDescPtr replace_concat = std::make_shared<OpDesc>("replace", CONCATV2);
  std::vector<std::vector<ge::NodePtr>> sorted_nodes = {{tile_node}};
  std::vector<std::pair<int32_t, ge::NodePtr>> concat_node_peer_output_nodes;
  ConcatTileFusionPass pass;
  EXPECT_EQ(fe::FAILED,
            pass.DoTileFusion(concat_node, *graph, replace_concat, sorted_nodes, concat_node_peer_output_nodes));
}

TEST_F(fusion_concat_tile_fusion_cast_st, remove_concat_edges_without_peer) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  OpDescPtr tile = std::make_shared<OpDesc>("tile", TILED);
  GeShape shape({1, 2});
  GeTensorDesc tensor_desc(shape, ge::FORMAT_NCHW, ge::DT_FLOAT);
  tile->AddInputDesc(tensor_desc);
  tile->AddOutputDesc(tensor_desc);
  NodePtr tile_node = graph->AddNode(tile);
  OpDescPtr concat = std::make_shared<OpDesc>("concat", CONCATV2);
  concat->AddInputDesc(tensor_desc);
  concat->AddOutputDesc(tensor_desc);
  NodePtr concat_node = graph->AddNode(concat);
  vector<ge::NodePtr> input_nodes = {tile_node};
  ConcatTileFusionPass pass;
  EXPECT_EQ(fe::FAILED, pass.RemoveConcatEdges(concat_node, input_nodes));
}

TEST_F(fusion_concat_tile_fusion_cast_st, sort_continuity_tile_nodes_without_peer) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  OpDescPtr tile = std::make_shared<OpDesc>("tile", TILED);
  GeShape shape({1, 2});
  GeTensorDesc tensor_desc(shape, ge::FORMAT_NCHW, ge::DT_FLOAT);
  tile->AddInputDesc(tensor_desc);
  tile->AddOutputDesc(tensor_desc);
  NodePtr tile_node = graph->AddNode(tile);
  std::vector<ge::NodePtr> tile_nodes = {tile_node};
  std::vector<std::vector<ge::NodePtr>> sorted_nodes;
  ConcatTileFusionPass pass;
  pass.SortContinuityTileNodes(tile_nodes, sorted_nodes);
  EXPECT_TRUE(sorted_nodes.empty());
}

TEST_F(fusion_concat_tile_fusion_cast_st, replace_concat_node_add_outputs_invalid_attr) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  OpDescPtr concat = std::make_shared<OpDesc>("concat", CONCATV2);
  GeShape shape({1, 2});
  GeTensorDesc tensor_desc(shape, ge::FORMAT_NCHW, ge::DT_FLOAT);
  concat->AddInputDesc(tensor_desc);
  concat->AddOutputDesc(tensor_desc);
  NodePtr concat_node = graph->AddNode(concat);
  OpDescPtr replace_concat = std::make_shared<OpDesc>("replace", CONCATV2);
  ConcatTileFusionPass pass;
  bool is_only_op = false;
  std::vector<ge::OutDataAnchorPtr> concat_node_peer_output_anchors;
  std::vector<ge::InDataAnchorPtr> concat_node_peer_input_anchors;
  EXPECT_EQ(fe::FAILED,
            pass.ReplaceConcatNodeAddOutputs(replace_concat, concat_node, is_only_op, concat_node_peer_output_anchors,
                                             concat_node_peer_input_anchors));
  (void)ge::AttrUtils::SetInt(replace_concat, "N", 1);
  EXPECT_EQ(fe::FAILED,
            pass.ReplaceConcatNodeAddOutputs(replace_concat, concat_node, is_only_op, concat_node_peer_output_anchors,
                                             concat_node_peer_input_anchors));
}

}  // namespace fe
