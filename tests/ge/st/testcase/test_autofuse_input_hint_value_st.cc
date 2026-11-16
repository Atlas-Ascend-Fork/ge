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
#include "api/atc/atc_option_map.h"
#include "api/atc/atc_flags.h"
#include "api/atc/main_impl.h"
#include "base/common/model/ge_root_model.h"
#include "compiler/graph/optimize/symbolic/infer_symbolic_shape/symbolic_infer_util.h"
#include "framework/common/framework_types_internal.h"
#include "ge_common/ge_common_api_types.h"
#include "ge_graph_dsl/graph_dsl.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/attr_utils.h"

namespace ge {
Status PrepareDualBodyHostInputs(const GeRootModelPtr &root_model);

class AutofuseInputHintValueST : public testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(AutofuseInputHintValueST, AtcOptionMapContainsInputHintValue) {
  const auto option_map = BuildAtcGeOptionToCliNameMap();
  const auto iter = option_map.find("ge.inputHintValue");
  ASSERT_NE(iter, option_map.end());
  EXPECT_EQ(iter->second, "--input_hint_value");
}

TEST_F(AutofuseInputHintValueST, MainImplRejectsInvalidInputHintValue) {
  char *argv[] = {"atc",
                  "--model=st_run_data/origin_model/add.pb",
                  "--output=./tmp_hint_value_cov",
                  "--framework=3",
                  "--soc_version=Ascend310",
                  "--input_format=NCHW",
                  "--input_hint_value=0:[-1]"};
  EXPECT_NE(main_impl(sizeof(argv) / sizeof(argv[0]), argv), 0);
  FLAGS_input_hint_value = "";
}

TEST_F(AutofuseInputHintValueST, PrepareDualBodyHostInputs) {
  DEF_GRAPH(host_input_graph) {
    CHAIN(NODE("data", DATA)->NODE("output", NETOUTPUT));
  };
  auto graph = ToComputeGraph(host_input_graph);
  ASSERT_NE(graph, nullptr);
  auto data = graph->FindNode("data");
  ASSERT_NE(data, nullptr);
  (void)AttrUtils::SetInt(data->GetOpDesc(), ATTR_NAME_INDEX, 0);
  (void)AttrUtils::SetListInt(graph, "_ge_need_symbolize_value_idxs", {0});

  auto root_model = std::make_shared<GeRootModel>();
  root_model->SetRootGraph(graph);
  EXPECT_EQ(PrepareDualBodyHostInputs(root_model), SUCCESS);
  std::vector<int64_t> host_idxs;
  ASSERT_TRUE(AttrUtils::GetListInt(graph, "_ge_host_input_idxs", host_idxs));
  EXPECT_EQ(host_idxs, std::vector<int64_t>({0}));
  bool host_input = false;
  ASSERT_TRUE(AttrUtils::GetBool(data->GetOpDesc(), ATTR_NAME_HOST_TENSOR_AS_MODEL_INPUT, host_input));
  EXPECT_TRUE(host_input);
}

}  // namespace ge
