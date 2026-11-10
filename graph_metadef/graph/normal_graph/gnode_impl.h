/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef METADEF_GRAPH_NORMAL_GRAPH_GNODE_IMPL_H_
#define METADEF_GRAPH_NORMAL_GRAPH_GNODE_IMPL_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "graph/gnode.h"
#include "graph/op_desc.h"

namespace ge {
class Node;
class FastNode;
class ComputeGraph;
class ExecuteGraph;
class Operator;
// NodeImpl：GNode 公共门面的多态状态层。类别即动态类型——计算/执行节点后端各自继承本基类，
// GNode 经 impl_ 调用公共流程，后端提供节点解析与 Operator 创建；
// 邻接与子图挂载操作由后端实现，节点引用晚绑定由 NodeAdapter 完成。
class NodeImpl {
 public:
  NodeImpl(const NodeImpl &) = delete;
  NodeImpl &operator=(const NodeImpl &) = delete;
  virtual ~NodeImpl() = default;

  graphStatus GetType(AscendString &type) const;
  graphStatus GetName(AscendString &name) const;
  virtual std::pair<GNodePtr, int32_t> GetInDataNodesAndPortIndexs(const int32_t index) const = 0;
  virtual std::vector<GNodePtr> GetInControlNodes() const = 0;
  virtual std::vector<GNodePtr> GetOutControlNodes() const = 0;
  virtual std::vector<std::pair<GNodePtr, int32_t>> GetOutDataNodesAndPortIndexs(const int32_t index) const = 0;
  graphStatus GetInputConstData(const int32_t index, Tensor &data) const;
  graphStatus GetInputIndexByName(const AscendString &name, int32_t &index) const;
  graphStatus GetOutputIndexByName(const AscendString &name, int32_t &index) const;
  graphStatus GetDynamicInputIndexesByName(const AscendString &name, std::vector<int32_t> &indexes) const;
  graphStatus GetDynamicOutputIndexesByName(const AscendString &name, std::vector<int32_t> &indexes) const;
  size_t GetInputsSize() const;
  size_t GetOutputsSize() const;
  // 端口描述合并入口：门面以 input 参数组合出 Get/UpdateInputDesc 与 Get/UpdateOutputDesc
  graphStatus GetDesc(const int32_t index, TensorDesc &tensor_desc, const bool input) const;
  graphStatus UpdateDesc(const int32_t index, const TensorDesc &tensor_desc, const bool input) const;

  // 属性接口：19+19 逐类型重载共享属性转换和返回值处理。
  graphStatus GetAttr(const AscendString &name, int64_t &attr_value) const;
  graphStatus GetAttr(const AscendString &name, int32_t &attr_value) const;
  graphStatus GetAttr(const AscendString &name, uint32_t &attr_value) const;
  graphStatus GetAttr(const AscendString &name, float32_t &attr_value) const;
  graphStatus GetAttr(const AscendString &name, AscendString &attr_value) const;
  graphStatus GetAttr(const AscendString &name, bool &attr_value) const;
  graphStatus GetAttr(const AscendString &name, Tensor &attr_value) const;
  graphStatus GetAttr(const AscendString &name, std::vector<int64_t> &attr_value) const;
  graphStatus GetAttr(const AscendString &name, std::vector<int32_t> &attr_value) const;
  graphStatus GetAttr(const AscendString &name, std::vector<uint32_t> &attr_value) const;
  graphStatus GetAttr(const AscendString &name, std::vector<float32_t> &attr_value) const;
  graphStatus GetAttr(const AscendString &name, std::vector<AscendString> &attr_value) const;
  graphStatus GetAttr(const AscendString &name, std::vector<bool> &attr_value) const;
  graphStatus GetAttr(const AscendString &name, std::vector<Tensor> &attr_value) const;
  graphStatus GetAttr(const AscendString &name, OpBytes &attr_value) const;
  graphStatus GetAttr(const AscendString &name, std::vector<std::vector<int64_t>> &attr_value) const;
  graphStatus GetAttr(const AscendString &name, std::vector<ge::DataType> &attr_value) const;
  graphStatus GetAttr(const AscendString &name, ge::DataType &attr_value) const;
  graphStatus GetAttr(const AscendString &name, AttrValue &attr_value) const;
  graphStatus SetAttr(const AscendString &name, int64_t &attr_value) const;
  graphStatus SetAttr(const AscendString &name, int32_t &attr_value) const;
  graphStatus SetAttr(const AscendString &name, uint32_t &attr_value) const;
  graphStatus SetAttr(const AscendString &name, float32_t &attr_value) const;
  graphStatus SetAttr(const AscendString &name, AscendString &attr_value) const;
  graphStatus SetAttr(const AscendString &name, bool &attr_value) const;
  graphStatus SetAttr(const AscendString &name, Tensor &attr_value) const;
  graphStatus SetAttr(const AscendString &name, std::vector<int64_t> &attr_value) const;
  graphStatus SetAttr(const AscendString &name, std::vector<int32_t> &attr_value) const;
  graphStatus SetAttr(const AscendString &name, std::vector<uint32_t> &attr_value) const;
  graphStatus SetAttr(const AscendString &name, std::vector<float32_t> &attr_value) const;
  graphStatus SetAttr(const AscendString &name, std::vector<AscendString> &attr_value) const;
  graphStatus SetAttr(const AscendString &name, std::vector<bool> &attr_value) const;
  graphStatus SetAttr(const AscendString &name, std::vector<Tensor> &attr_value) const;
  graphStatus SetAttr(const AscendString &name, OpBytes &attr_value) const;
  graphStatus SetAttr(const AscendString &name, std::vector<std::vector<int64_t>> &attr_value) const;
  graphStatus SetAttr(const AscendString &name, std::vector<ge::DataType> &attr_value) const;
  graphStatus SetAttr(const AscendString &name, ge::DataType &attr_value) const;
  graphStatus SetAttr(const AscendString &name, AttrValue &attr_value) const;

  graphStatus GetAllAttrs(std::map<AscendString, AttrValue> &attr_values) const;
  graphStatus SetAttrs(const std::map<AscendString, AttrValue> &attr_values) const;
  bool HasAttr(const AscendString &name) const;

  graphStatus GetSubgraph(const uint32_t index, GraphPtr &graph) const;
  graphStatus GetAllSubgraphs(std::vector<GraphPtr> &graph_list) const;
  virtual graphStatus SetSubgraph(const AscendString &subgraph_ir_name, const Graph &subgraph) const = 0;
  virtual graphStatus SetSubgraphs(const AscendString &subgraph_ir_name, const std::vector<Graph> &subgraphs) const = 0;
  // 端口属性合并入口：input 选择输入/输出端口
  graphStatus PortAttr(const AscendString &name, const uint32_t index, AttrValue &attr_value, const bool input) const;
  graphStatus SetPortAttr(const AscendString &name, const uint32_t index, const AttrValue &attr_value,
                          const bool input) const;

 protected:
  NodeImpl() = default;
  // 保持本次访问所需的节点或图存活；执行节点每次解析时校验句柄。
  struct NodeContext {
    std::shared_ptr<Node> compute_node;
    std::shared_ptr<ExecuteGraph> execute_owner;
    FastNode *execute_node = nullptr;
    OpDescPtr op_desc;

    bool IsValid() const;
    OpDescPtr GetOpDesc() const {
      return op_desc;
    }
    OpDesc *GetOpDescBarePtr() const {
      return op_desc.get();
    }
    std::string GetName() const;
    std::string GetType() const;
    const char *GetNamePtr() const;
    const char *GetTypePtr() const;
  };

  struct GraphContext {
    std::shared_ptr<ComputeGraph> compute_graph;
    std::shared_ptr<ExecuteGraph> execute_graph;

    bool IsValid() const {
      return compute_graph != nullptr || execute_graph != nullptr;
    }
  };

  virtual NodeContext ResolveNode() const = 0;
  virtual Operator CreateOperator(const NodeContext &node) const = 0;
  virtual GraphContext LookupSubgraph(const NodeContext &node, uint32_t index) const = 0;
  virtual GraphContext LookupRootGraph(const NodeContext &node) const = 0;
  virtual std::vector<GraphContext> ListSubgraphs(const GraphContext &root) const = 0;
  virtual GraphPtr WrapGraph(const GraphContext &graph) const = 0;
  // 成功时返回常量来源；缺少输入与非常量输入分别返回 PARAM_INVALID 和 GRAPH_NODE_WITHOUT_CONST_INPUT。
  virtual graphStatus ResolveConstInput(const NodeContext &node, int32_t index, NodeContext &source) const = 0;

 private:
  graphStatus GetInputDescInner(const int32_t index, TensorDesc &tensor_desc) const;
  graphStatus GetOutputDescInner(const int32_t index, TensorDesc &tensor_desc) const;
  graphStatus UpdateInputDescInner(const int32_t index, const TensorDesc &tensor_desc) const;
  graphStatus UpdateOutputDescInner(const int32_t index, const TensorDesc &tensor_desc) const;
  graphStatus GetPortAttrValue(const NodeContext &node, const AscendString &name, const uint32_t index,
                               AttrValue &attr_value, const bool is_input) const;
  graphStatus SetPortAttrValue(const NodeContext &node, const AscendString &name, const uint32_t index,
                               const AttrValue &attr_value, const bool is_input) const;
};
}  // namespace ge
#endif  // METADEF_GRAPH_NORMAL_GRAPH_GNODE_IMPL_H_
