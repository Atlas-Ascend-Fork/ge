/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <vector>

#include "ge/ge_api.h"
#include "graph.h"
#include "ops_proto_legacy.h"
#include "tensor.h"
#include "types.h"
#include "concat_v2_custom.h"

using ge::Operator;

namespace {
constexpr uint32_t kGraphId = 0U;
constexpr int64_t kDynamicInputNum = 2;

// 两路输入: x0 = [2,2,2] (1..8), x1 = [2,1,2] (9..12)，沿 axis=1 拼接后 y = [2,3,2]
ge::TensorDesc BuildX0Desc() {
  return ge::TensorDesc(ge::Shape({2, 2, 2}), ge::FORMAT_ND, ge::DT_FLOAT);
}

ge::TensorDesc BuildX1Desc() {
  return ge::TensorDesc(ge::Shape({2, 1, 2}), ge::FORMAT_ND, ge::DT_FLOAT);
}

ge::op::Data BuildDataOp(const char *name, const ge::TensorDesc &desc) {
  auto data = ge::op::Data(name);
  data.update_input_desc_x(desc);
  data.update_output_desc_y(desc);
  return data;
}

// concat_dim 为 Data 输入（host 放置，与生产算子 GEIR 测试一致）：
// 编译期值不可得保秩退化，运行期由框架物化 host 副本后经执行图 InferShape 节点重推精确 shape
std::unique_ptr<ge::Graph> BuildGraph() {
  const auto x0_desc = BuildX0Desc();
  const auto x1_desc = BuildX1Desc();
  auto x0 = BuildDataOp("x0", x0_desc);
  auto x1 = BuildDataOp("x1", x1_desc);

  ge::TensorDesc concat_dim_desc(ge::Shape({1}), ge::FORMAT_ND, ge::DT_INT32);
  concat_dim_desc.SetPlacement(ge::kPlacementHost);
  auto concat_dim = BuildDataOp("concat_dim", concat_dim_desc);

  auto concat = ge::op::ConcatV2Custom("concat_v2");
  // 头部插入，保持输入 flat 顺序与 IR 声明顺序一致（默认尾部追加会导致索引错位）
  concat.create_dynamic_input_x(kDynamicInputNum, false);
  concat.update_dynamic_input_desc_x(0, x0_desc);
  concat.update_dynamic_input_desc_x(1, x1_desc);
  concat.update_input_desc_concat_dim(concat_dim_desc);
  concat.set_dynamic_input_x(0, x0);
  concat.set_dynamic_input_x(1, x1);
  concat.set_input_concat_dim(concat_dim);
  concat.set_attr_N(kDynamicInputNum);

  std::vector<Operator> inputs;
  inputs.emplace_back(x0);
  inputs.emplace_back(x1);
  inputs.emplace_back(concat_dim);
  std::vector<Operator> outputs;
  outputs.emplace_back(concat);

  auto graph = std::make_unique<ge::Graph>("ValueDependentConcatGraph");
  graph->SetInputs(inputs).SetOutputs(outputs);
  return graph;
}

ge::Tensor BuildX0Tensor() {
  ge::TensorDesc x0_desc = BuildX0Desc();
  ge::Tensor x0_tensor(x0_desc);
  const std::vector<float> data = {1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F, 7.0F, 8.0F};
  if (x0_tensor.SetData(reinterpret_cast<const uint8_t *>(data.data()), data.size() * sizeof(float)) !=
      ge::GRAPH_SUCCESS) {
    std::cerr << "SetData failed" << std::endl;
  }
  return x0_tensor;
}

ge::Tensor BuildX1Tensor() {
  ge::TensorDesc x1_desc = BuildX1Desc();
  ge::Tensor x1_tensor(x1_desc);
  const std::vector<float> data = {9.0F, 10.0F, 11.0F, 12.0F};
  if (x1_tensor.SetData(reinterpret_cast<const uint8_t *>(data.data()), data.size() * sizeof(float)) !=
      ge::GRAPH_SUCCESS) {
    std::cerr << "SetData failed" << std::endl;
  }
  return x1_tensor;
}

ge::Tensor BuildConcatDimTensor() {
  ge::TensorDesc concat_dim_desc(ge::Shape({1}), ge::FORMAT_ND, ge::DT_INT32);
  concat_dim_desc.SetPlacement(ge::kPlacementHost);
  ge::Tensor concat_dim_tensor(concat_dim_desc);
  const std::vector<int32_t> concat_dim_value = {1};
  if (concat_dim_tensor.SetData(reinterpret_cast<const uint8_t *>(concat_dim_value.data()),
                                concat_dim_value.size() * sizeof(int32_t)) != ge::GRAPH_SUCCESS) {
    std::cerr << "SetData failed" << std::endl;
  }
  return concat_dim_tensor;
}

void PrintOutputTensor(const ge::Tensor &output_tensor) {
  const auto tensor_desc = output_tensor.GetTensorDesc();
  const auto shape = tensor_desc.GetShape();
  const auto dims = shape.GetDims();
  std::cout << "output shape: [";
  for (size_t i = 0U; i < dims.size(); ++i) {
    if (i != 0U) {
      std::cout << ", ";
    }
    std::cout << dims[i];
  }
  std::cout << "]" << std::endl;

  int64_t element_count = shape.GetShapeSize();
  if (element_count <= 0) {
    element_count = static_cast<int64_t>(output_tensor.GetSize() / sizeof(float));
  }
  const auto *output_data = reinterpret_cast<const float *>(output_tensor.GetData());
  std::cout << "output values:";
  for (int64_t i = 0; i < element_count; ++i) {
    std::cout << " " << output_data[i];
  }
  std::cout << std::endl;
}
}  // namespace

namespace {
bool RunScenario(ge::Session &session, const uint32_t graph_id, const std::unique_ptr<ge::Graph> &graph,
                 const std::vector<ge::Tensor> &inputs, const char *scenario_name) {
  std::cout << "=== " << scenario_name << " ===" << std::endl;
  if (graph == nullptr) {
    std::cerr << "BuildGraph failed" << std::endl;
    return false;
  }
  const auto add_graph_ret = session.AddGraph(graph_id, *graph);
  if (add_graph_ret != ge::SUCCESS) {
    std::cerr << "AddGraph failed, ret: " << add_graph_ret << std::endl;
    return false;
  }
  std::vector<ge::Tensor> outputs;
  const auto run_ret = session.RunGraph(graph_id, inputs, outputs);
  bool scenario_ok = false;
  if (run_ret != ge::SUCCESS) {
    std::cerr << "RunGraph failed, ret: " << run_ret << std::endl;
  } else if (outputs.empty()) {
    std::cerr << "RunGraph success but outputs is empty" << std::endl;
  } else {
    PrintOutputTensor(outputs[0]);
    scenario_ok = true;
  }
  (void)session.RemoveGraph(graph_id);
  return scenario_ok;
}
}  // namespace

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  std::map<ge::AscendString, ge::AscendString> options = {
      {"ge.exec.deviceId", "0"},
      // 动态图执行模式（与生产算子 GEIR 测试一致）
      {"ge.graphRunMode", "1"},
  };

  const auto init_ret = ge::GEInitialize(options);
  if (init_ret != ge::SUCCESS) {
    std::cerr << "GEInitialize failed, ret: " << init_ret << std::endl;
    return 1;
  }

  int ret_code = 0;
  {
    ge::Session session(options);

    // concat_dim 为 Data 输入，轴值运行期才喂入
    const bool run_ok =
        RunScenario(session, kGraphId, BuildGraph(), {BuildX0Tensor(), BuildX1Tensor(), BuildConcatDimTensor()},
                    "concat_dim as Data input (axis value read at execution time)");
    if (!run_ok) {
      ret_code = 1;
    }
  }

  const auto finalize_ret = ge::GEFinalize();
  if (finalize_ret != ge::SUCCESS) {
    std::cerr << "GEFinalize failed, ret: " << finalize_ret << std::endl;
    return 1;
  }
  return ret_code;
}
