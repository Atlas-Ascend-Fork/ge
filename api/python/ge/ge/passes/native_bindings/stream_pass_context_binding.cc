/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "pass_bindings.h"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "binding_log.h"
#include "binding_utils.h"
#include "graph/graph.h"
#include "register/register_custom_pass.h"

namespace ge {
namespace python_pass_native {
namespace {
const Graph *GetGraphFromPythonObject(const py::object &graph_object) {
  if (graph_object.is_none()) {
    throw std::runtime_error("graph must not be None");
  }
  // ctypes returns integer addresses for c_void_p array entries (subgraphs),
  // while borrowed root graphs carry c_void_p objects. Use the shared parser.
  const auto *graph = BorrowGraphFromPython(graph_object);
  if (graph == nullptr) {
    throw std::runtime_error("graph handle is null");
  }
  return graph;
}

std::vector<GNode> GetDirectNodes(const py::object &graph_object) {
  // Graph/Node are borrowed wrappers.  Resolve the direct-node list for each
  // operation so an index can never be applied to a different graph.  This
  // is intentionally O(V) per call (and therefore O(V^2) for a full read /
  // write pass); stream graphs are currently small and profiling should drive
  // any future bulk API reconsideration.
  const auto *graph = GetGraphFromPythonObject(graph_object);
  const auto nodes = graph->GetDirectNode();
  return nodes;
}

size_t ParseNodeIndex(int64_t index, const std::vector<GNode> &nodes) {
  if (index < 0 || static_cast<uint64_t>(index) >= nodes.size()) {
    throw std::runtime_error("Graph node index out of range");
  }
  return static_cast<size_t>(index);
}

void SetStreamId(StreamPassContext &context, const py::object &graph_object, int64_t index, int64_t stream_id) {
  const auto nodes = GetDirectNodes(graph_object);
  const auto node_index = ParseNodeIndex(index, nodes);
  const auto ret = context.SetStreamId(nodes[node_index], stream_id);
  if (ret != GRAPH_SUCCESS) {
    // Same fields as the former C++ DAGAdapter error: node name, stream id and return code.
    AscendString name;
    (void)nodes[node_index].GetName(name);
    const std::string message =
        "SetStreamId failed for node " + std::string((name.GetString() == nullptr) ? "" : name.GetString()) +
        ", stream_id=" + std::to_string(stream_id) + ", ret=" + std::to_string(static_cast<int32_t>(ret));
    GE_PYPASS_LOGE("%s", message.c_str());
    throw std::runtime_error(message);
  }
}

int64_t GetStreamId(StreamPassContext &context, const py::object &graph_object, int64_t index) {
  const auto nodes = GetDirectNodes(graph_object);
  const auto node_index = ParseNodeIndex(index, nodes);
  return context.GetStreamId(nodes[node_index]);
}

}  // namespace

void BindStreamPassContext(py::module_ &m) {
  py::class_<StreamPassContext, CustomPassContext>(m, "StreamPassContext", "Stream pass context")
      .def("set_stream_id", &SetStreamId, py::arg("graph"), py::arg("node_index"), py::arg("stream_id"))
      .def("get_stream_id", &GetStreamId, py::arg("graph"), py::arg("node_index"))
      .def("allocate_next_stream_id", &StreamPassContext::AllocateNextStreamId)
      .def("get_curr_max_stream_id", &StreamPassContext::GetCurrMaxStreamId);
}
}  // namespace python_pass_native
}  // namespace ge
