/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef METADEF_GRAPH_NORMAL_GRAPH_GRAPH_IMPL_H_
#define METADEF_GRAPH_NORMAL_GRAPH_GRAPH_IMPL_H_

#include <iosfwd>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "common/checker.h"
#include "graph/compute_graph.h"
#include "graph/ge_attr_value.h"
#include "graph/graph.h"
#include "graph/gnode.h"

namespace ge {
// GraphImpl：Graph 公共门面的多态状态层。类别即动态类型——计算/执行图后端各自继承本基类，
// 门面经 impl_ 虚调用分发，不持有 kind 字段与路由分支；接入新图类别只需新增派生类。
// 门面层保留公共对象检查（impl_ 判空）与链式接口形式适配，正文全部在后端实现。
class GraphImpl {
 public:
  GraphImpl(const GraphImpl &) = delete;
  GraphImpl &operator=(const GraphImpl &) = delete;
  virtual ~GraphImpl() = default;

  virtual bool IsValid() const = 0;
  virtual const std::string &GetName() const = 0;
  // 首次建图：创建空 ComputeGraph（原 GraphImpl::InitComputeGraph）
  virtual graphStatus InitGraph() = 0;
  // 惰性建图模板方法（非虚）：组合 IsValid/InitGraph 虚原语，Graph::SetValid 与后端建图路径共用
  graphStatus SetValid() {
    if (IsValid()) {
      return GRAPH_SUCCESS;
    }
    GELOGD("Inner graph is invalid, start to init at first");
    return InitGraph();
  }
  // 工具型虚接口：供 GraphUtils/GraphUtilsEx 按类型访问；执行类别实现时返回空或拒绝
  virtual ComputeGraphPtr GetComputeGraph() const = 0;
  // 桥接入口整体挂入既有计算图（原 compute_graph_ 直赋值语义）
  virtual void AdoptGraph(const ComputeGraphPtr &compute_graph) = 0;
  virtual graphStatus RecoverOperators() = 0;
  virtual graphStatus CopyOperatorsTo(GraphImpl &dst, const std::map<ConstNodePtr, NodePtr> &node_old_2_new,
                                      const std::map<ConstOpDescPtr, OpDescPtr> &op_desc_old_2_new) const = 0;

  // ---- IR 构图（Operator 系） ----
  virtual graphStatus SetInputs(const std::vector<Operator> &inputs) = 0;
  virtual graphStatus SetOutputs(const std::vector<Operator> &outputs) = 0;
  virtual graphStatus SetOutputs(const std::vector<std::pair<Operator, std::vector<size_t>>> &output_indexs) = 0;
  virtual graphStatus SetOutputs(const std::vector<std::pair<Operator, std::string>> &outputs) = 0;
  virtual graphStatus SetOutputs(const std::vector<std::pair<GNode, int32_t>> &outputs) = 0;
  virtual graphStatus SetTargets(const std::vector<Operator> &targets) = 0;
  virtual graphStatus AddOp(const ge::Operator &op) = 0;
  virtual graphStatus FindOpByName(const std::string &name, ge::Operator &op) const = 0;
  virtual graphStatus FindOpByType(const std::string &type, std::vector<ge::Operator> &ops) const = 0;
  virtual graphStatus GetAllOpName(std::vector<std::string> &op_name) const = 0;
  virtual void SetNeedIteration(bool need_iteration) = 0;

  // ---- 节点/边编辑（GNode 系） ----
  virtual std::vector<GNode> GetAllNodes() const = 0;
  virtual std::vector<GNode> GetDirectNode() const = 0;
  virtual GNode AddNodeByOp(const Operator &op) = 0;
  virtual graphStatus AddDataEdge(GNode &src_node, const int32_t src_port_index, GNode &dst_node,
                                  const int32_t dst_port_index) = 0;
  virtual graphStatus AddControlEdge(GNode &src_node, GNode &dst_node) = 0;
  virtual graphStatus RemoveNode(GNode &node, bool contain_subgraph) = 0;
  virtual graphStatus RemoveEdge(GNode &src_node, const int32_t src_port_index, GNode &dst_node,
                                 const int32_t dst_port_index) = 0;
  virtual GNodePtr FindNodeByName(const AscendString &node_name) const = 0;

  // ---- 子图 ----
  virtual std::vector<GraphPtr> GetAllSubgraphs() const = 0;
  virtual GraphPtr GetSubGraph(const char *name) const = 0;
  virtual graphStatus AddSubGraph(const Graph &subgraph) = 0;
  virtual graphStatus RemoveSubgraph(const char *name) = 0;
  virtual GraphPtr GetParentGraph() const = 0;
  virtual GNodePtr GetParentNode() const = 0;

  // ---- 属性 ----
  virtual graphStatus SetAttr(const AscendString &name, const AttrValue &attr_value) = 0;
  virtual graphStatus GetAttr(const AscendString &name, AttrValue &attr_value) const = 0;

  // ---- Dump ----
  virtual graphStatus Dump(Graph::DumpFormat format, std::ostream &o_stream) const = 0;
  virtual graphStatus DumpToFile(Graph::DumpFormat format, const AscendString &suffix) const = 0;

 protected:
  GraphImpl() = default;
};
}  // namespace ge
#endif  // METADEF_GRAPH_NORMAL_GRAPH_GRAPH_IMPL_H_
