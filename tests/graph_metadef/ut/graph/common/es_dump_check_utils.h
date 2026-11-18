/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

/*
 * ES/Graph 执行图 dump 验证公共工具（EsFacadeDumpTest / EsFacadeDumpIfaceTest 共用）。
 * 三层验证模型辅助：L2 readable/kTxt 文本断言、L3 kTxt→ModelDef→Model 回环。
 * dump 为 adapter 只读快照（OutNodeInfo/target 有损区不在此断言）。
 */

#ifndef METADEF_UT_GRAPH_COMMON_ES_DUMP_CHECK_UTILS_H_
#define METADEF_UT_GRAPH_COMMON_ES_DUMP_CHECK_UTILS_H_

#include <algorithm>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "graph/ascend_string.h"
#include "graph/graph.h"
#include "graph/gnode.h"
#include "graph/model.h"
#include "graph/node.h"
#include "ge_ir.pb.h"
#include <google/protobuf/text_format.h>

namespace ge {
namespace ut {
namespace esdump {

// L2：dump 为 readable 文本（不落盘，规避 DUMP_GRAPH_PATH 进程级静态缓存）。
inline std::string DumpReadable(const Graph &graph) {
  std::stringstream ss;
  EXPECT_EQ(GRAPH_SUCCESS, graph.Dump(Graph::DumpFormat::kReadable, ss));
  return ss.str();
}

// L2/L3 辅助：dump 为 kTxt 文本协议（含端口描述/属性，readable 不渲染描述时用此口径）。
inline std::string DumpTxt(const Graph &graph) {
  std::stringstream ss;
  EXPECT_EQ(GRAPH_SUCCESS, graph.Dump(Graph::DumpFormat::kTxt, ss));
  return ss.str();
}

// L2 辅助：提取 readable 产物中的节点行（"%<name> : ..." 开头），name → 整行。
inline std::map<std::string, std::string> CollectNodeLines(const std::string &readable) {
  std::map<std::string, std::string> lines;
  std::istringstream iss(readable);
  std::string line;
  while (std::getline(iss, line)) {
    const auto pos = line.find("%");
    if (pos == std::string::npos) {
      continue;
    }
    const auto name_end = line.find(" :", pos);
    if (name_end == std::string::npos) {
      continue;
    }
    lines[line.substr(pos + 1UL, name_end - pos - 1UL)] = line;
  }
  return lines;
}

// L3 辅助：kTxt 流 → ModelDef 文本协议 → Model::Load → 回读 ComputeGraph。
inline ComputeGraphPtr RoundTripFromTxtDump(const Graph &graph) {
  std::stringstream ss;
  EXPECT_EQ(GRAPH_SUCCESS, graph.Dump(Graph::DumpFormat::kTxt, ss));
  ge::proto::ModelDef model_def;
  EXPECT_TRUE(google::protobuf::TextFormat::ParseFromString(ss.str(), &model_def));
  Model model;
  EXPECT_EQ(SUCCESS, model.Load(model_def));
  EXPECT_NE(nullptr, model.GetGraph());
  return model.GetGraph();
}

// L3 辅助：回读图中指定类型的节点数。
template <typename ContainerT>
size_t CountTypeOf(const ContainerT &nodes, const std::string &type) {
  return std::count_if(nodes.begin(), nodes.end(), [&type](const auto &node) { return node->GetType() == type; });
}

// L3 辅助：节点名集合。
template <typename ContainerT>
std::set<std::string> NamesOf(const ContainerT &nodes) {
  std::set<std::string> names;
  for (const auto &node : nodes) {
    if (node != nullptr) {
      names.insert(node->GetName());
    }
  }
  return names;
}

// L1/L2 辅助：GNode 名称读回。
inline std::string GNodeNameOf(const GNode &node) {
  AscendString name;
  return (node.GetName(name) == GRAPH_SUCCESS) ? std::string(name.GetString()) : std::string("<null>");
}

inline std::string GNodeNameOf(const GNodePtr &node) {
  AscendString name;
  return ((node != nullptr) && (node->GetName(name) == GRAPH_SUCCESS)) ? std::string(name.GetString())
                                                                       : std::string("<null>");
}

// 不变性断言（拒绝类接口）：调用前后 readable dump 逐字节一致。
// 拒绝返回值由调用方在 callable 内自行断言（SetNeedIteration 等无返回值接口不强制）。
template <typename CallableT>
void ExpectReadableUnchangedAround(const Graph &graph, CallableT &&call) {
  const auto before = DumpReadable(graph);
  call();
  EXPECT_EQ(before, DumpReadable(graph)) << "readable dump changed around rejected call";
}

// 不变性断言（描述设置器拦截等）：调用前后 kTxt dump（含端口描述）逐字节一致。
template <typename CallableT>
void ExpectTxtUnchangedAround(const Graph &graph, CallableT &&call) {
  const auto before = DumpTxt(graph);
  call();
  EXPECT_EQ(before, DumpTxt(graph)) << "kTxt dump changed around rejected call";
}

}  // namespace esdump
}  // namespace ut
}  // namespace ge

#endif  // METADEF_UT_GRAPH_COMMON_ES_DUMP_CHECK_UTILS_H_
